/*
 * BasicPlus — ESP32-P4: VM.3 — "Run on P4" desde el IDE (wire v1 sobre TCP).
 *
 * Sobre el bring-up de red (Ethernet IP101 + IP estática 192.168.2.2) y la
 * VM-C ya verificada en RISC-V (VM.1/VM.2), el P4 pasa de ejecutar un módulo
 * EMBEBIDO a ser un TARGET de desarrollo: levanta un SERVIDOR del wire BPVM v1
 * sobre TCP y el IDE (backend "VM (TCP v1)") sube/ejecuta apps arbitrarias.
 *
 * Dos canales TCP, independientes:
 *   - P4 -> PC 192.168.2.1:5555  (cliente)  = log de bring-up (net_logf), para
 *     ver el lado servidor con pc_logserver.py por si hay líos de comms.
 *   - IDE -> P4 *:3333           (servidor) = wire v1 (HELLO/PUT/RUN/KILL...).
 *     El dispatcher es repl_esp32.c REUTILIZADO TAL CUAL (agnóstico del
 *     transporte); el I/O de bytes va por wire_v1_tcp.c (sockets lwIP).
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

#include "fs.h"               /* FS-RAM (VM.2b) + fs_register_bpvm (#247) */
#include "p4_mods.h"          /* stdlib embebida (Core fresco) -> /lib */
#include "repl_esp32.h"       /* dispatcher wire v1 REUTILIZADO (agnóstico transporte) */
#include "wire_v1_tcp.h"      /* servidor del wire sobre TCP (capa de I/O del P4) */
#include "p4_board_id.h"      /* identidad de placa esp32p4 para INFO/HELLO */
#include "gui_display_dsi.h"  /* G3: display MIPI-DSI (panel + backlight + rojo) */
#include "hw_esp32.h"         /* H14: backends HW (GPIO/UART/SPI/I2C) reusados del S3 */

static const char *TAG = "bpvm_p4";

#define SERVER_IP    "192.168.2.1"
#define SERVER_PORT  5555           /* log de bring-up (P4 cliente -> PC) */
#define WIRE_PORT    3333           /* wire v1 (IDE cliente -> P4 servidor) */
#define LINK_UP_BIT  BIT0

/* Memoria de la VM (caller-provided). repl_esp32.c la referencia como extern
 * (misma convención que el S3): NO static. 128 KiB en RAM interna — los
 * programas grandes irán a PSRAM más adelante. */
#define VM_MEM_SIZE  (128 * 1024)
uint8_t        s_vm_buffer[VM_MEM_SIZE];
const uint32_t s_vm_buffer_size = VM_MEM_SIZE;

static EventGroupHandle_t s_events;
static int s_sock = -1;     /* socket del canal de log (lo usa net_logf) */

/* Log instrumentado: a consola (idf.py monitor) Y al socket TCP de log. El
 * caller NO pone '\n' final; lo añade esta función para el lado TCP. NO static:
 * wire_v1_tcp.c lo usa por extern para trazar accept/disconnect/errores. */
void net_logf(const char *fmt, ...)
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

/* ---- Ethernet (init verificada en bring-up: IP101 / RMII, pines EV board) ---- */
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
    (void) arg; (void) base; (void) data;
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
    (void) arg; (void) base; (void) id;
    ip_event_got_ip_t *e = (ip_event_got_ip_t *) data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&e->ip_info.ip));
}

/* ---- Canal de log de bring-up: P4 cliente -> pc_logserver.py:5555 ----
 * Solo diagnóstico (el wire lleva la salida de los programas al IDE). Si el PC
 * no escucha, reintenta; el firmware funciona igual sin él. */
static void tcp_log_task(void *arg)
{
    (void) arg;
    xEventGroupWaitBits(s_events, LINK_UP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));

    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) { vTaskDelay(pdMS_TO_TICKS(2000)); continue; }
        struct sockaddr_in dest = {0};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(SERVER_PORT);
        dest.sin_addr.s_addr = inet_addr(SERVER_IP);

        if (connect(sock, (struct sockaddr *) &dest, sizeof(dest)) == 0) {
            s_sock = sock;
            net_logf("[p4] canal de log conectado a %s:%d (bring-up)", SERVER_IP, SERVER_PORT);
            int n = 0;
            while (1) {
                net_logf("[p4] idle %d uptime=%lld ms", n++,
                         (long long)(esp_timer_get_time() / 1000));
                vTaskDelay(pdMS_TO_TICKS(5000));
                if (send(sock, "", 0, 0) < 0) break;   /* detecta caída del log */
            }
            s_sock = -1;
        }
        close(sock);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ---- Servidor del wire v1: la VM corre AQUÍ (stack holgado). ----
 * Prepara FS + stdlib (independiente del canal de log), abre el socket de
 * escucha y entra al bucle del REPL reutilizado. No retorna. */
static void wire_task(void *arg)
{
    (void) arg;
    xEventGroupWaitBits(s_events, LINK_UP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    /* FS + stdlib listos ANTES de aceptar al IDE (no dependen del PC log). */
    fs_status_t r = fs_init();
    fs_register_bpvm();          /* #247: file I/O desde BP sobre este FS */
    esp32p4_mods_install();      /* Core fresco -> /lib (if-absent) */
    net_logf("[p4] FS+stdlib listos: %s, %d ficheros, %u B libres",
             fs_status_str(r), fs_file_count(), (unsigned) fs_free_bytes());

    esp32_hw_register();         /* H14: backends GPIO/UART/SPI/I2C (reúso del S3, ESP-IDF) */
    p4_install_board_id();       /* INFO/HELLO esp32p4 + Pico.* backend del P4 (pisa el del S3) */
    wire_v1_tcp_server_init(WIRE_PORT);
    net_logf("[p4] VM.3: esperando al IDE en *:%d (backend 'VM (TCP v1)' -> 192.168.2.2:%d)",
             WIRE_PORT, WIRE_PORT);

    /* /sys/auto.txt si existiera (no en P4 todavía) + bucle del REPL. El
     * accept/reconnect vive dentro de wire_v1_recv_line (wire_v1_tcp.c). */
    repl_esp32_autorun();
    repl_esp32_run();            /* no retorna */
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

    /* Canal de log (diagnóstico) — stack pequeño, no corre la VM. */
    xTaskCreate(tcp_log_task, "tcp_log", 4096, NULL, 5, NULL);
    /* Servidor del wire — la VM corre dentro y, con Gui.run(), también el render
     * LVGL → stack holgado (32 KiB). */
    xTaskCreate(wire_task, "wire_v1", 32768, NULL, 5, NULL);

    ESP_LOGI(TAG, "P4 VM.3: IP 192.168.2.2 | log %s:%d | wire v1 TCP *:%d",
             SERVER_IP, SERVER_PORT, WIRE_PORT);

    /* G6 — la GUI la dirige BasicPlus: cuando un .mod usa Gui.*, gui.c hace lv_init()
     * + bpvm_gui_disp_init() (costura en gui_display_dsi.c) y Gui.run() bombea, todo
     * dentro del VM (wire_task). El panel/táctil arrancan en ese momento; app_main
     * solo deja red + tasks listas. (p4_gfx_lvgl_test/smoke quedan de diagnóstico.) */
}
