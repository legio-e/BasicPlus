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
#include <stdlib.h>           /* abort() si la reserva PSRAM falla */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"   /* heap_caps_malloc: heap de la VM en PSRAM */
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "fs.h"               /* FS-RAM (VM.2b) + fs_register_bpvm (#247) */
#include "esp32_mods.h"       /* stdlib embebida (14 mods, compartida con el S3) -> /lib */
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
 * (PUNTERO, misma convención que el S3 y la Pico/Metro). En el P4 el heap de la
 * VM vive en PSRAM (32 MB in-package, HEX@200 MHz → baja penalización): se
 * reserva en vm_buffer_init_psram() antes de arrancar las tasks. El P4 SIEMPRE
 * lleva PSRAM (el framebuffer del display también la usa). */
#define VM_MEM_SIZE  (2 * 1024 * 1024)   /* 2 MiB en PSRAM (antes 128 KiB SRAM) */
uint8_t*       s_vm_buffer      = NULL;
uint32_t       s_vm_buffer_size = 0;

static EventGroupHandle_t s_events;   /* gate de Link Up (Ethernet — ambos transportes) */
static int s_sock = -1;     /* socket del canal de log (lo usa net_logf, común) */

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

/* ===== Ethernet — COMÚN a ambos transportes. En TCP lleva el wire (:3333); en
 * UART lleva SOLO los logs (net_logf → :5555) como red de seguridad. (Init
 * verificada en bring-up: IP101 / RMII, pines EV board.) ===== */
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

#ifdef BPVM_P4_WIRE_TCP
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
    esp32_mods_install();        /* stdlib completa -> /lib (if-absent, save en lote) */
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

#else   /* !BPVM_P4_WIRE_TCP — transporte por defecto */

/* ===== Transporte UART0 (#138): el IDE conecta al puerto del bridge USB-UART, SIN
 * red → sirve también a placas sin Ethernet (la nueva con pantalla). Gemelo de
 * wire_task: mismo FS+stdlib+HW y el mismo repl_esp32_run reutilizado, pero NO
 * espera Link Up y arranca wire_v1_uart_init en vez del servidor TCP. ===== */
static void wire_task_uart(void *arg)
{
    (void) arg;
    /* Espera Link Up (con timeout) para que net_logf (red de seguridad) llegue al
     * PC antes de arrancar. Si la placa no tiene Ethernet, tras el timeout arranca
     * igual (sin logs por red). */
    xEventGroupWaitBits(s_events, LINK_UP_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));

    fs_status_t r = fs_init();
    fs_register_bpvm();          /* #247: file I/O desde BP sobre este FS */
    esp32_mods_install();        /* stdlib completa -> /lib (if-absent, save en lote) */
    net_logf("[p4] FS+stdlib listos (UART): %s, %d ficheros, %u B libres",
             fs_status_str(r), fs_file_count(), (unsigned) fs_free_bytes());

    esp32_hw_register();         /* H14: backends GPIO/UART/SPI/I2C (reúso del S3) */
    p4_install_board_id();       /* INFO/HELLO esp32p4 + Pico.* del P4 */
    wire_v1_uart_init();
    net_logf("[p4] VM.3 (UART0): wire v1 por el bridge USB-UART; conecta el IDE");

    repl_esp32_autorun();        /* /sys/auto.txt si existiera */
    repl_esp32_run();            /* no retorna */
}

#endif  /* BPVM_P4_WIRE_TCP */

/* Reserva el heap de la VM en PSRAM. El P4 SIEMPRE lleva PSRAM (in-package, la
 * comparte con el framebuffer del display); si la reserva falla el firmware no es
 * operativo → abort con mensaje claro (mejor que arrancar y colgar luego en el
 * primer programa grande). Los firmwares sin PSRAM (S3/Pico/STM32) mantienen su
 * buffer en SRAM; esto es board-specific del P4. */
static void vm_buffer_init_psram(void)
{
    s_vm_buffer = (uint8_t*) heap_caps_malloc(VM_MEM_SIZE, MALLOC_CAP_SPIRAM);
    if (s_vm_buffer == NULL) {
        ESP_LOGE(TAG, "PSRAM: no se pudo reservar el heap de la VM (%u KiB) - abort",
                 (unsigned)(VM_MEM_SIZE / 1024u));
        abort();
    }
    s_vm_buffer_size = VM_MEM_SIZE;
    ESP_LOGI(TAG, "VM heap en PSRAM: %u KiB @ %p",
             (unsigned)(VM_MEM_SIZE / 1024u), (void*) s_vm_buffer);
}

void app_main(void)
{
    /* Heap de la VM en PSRAM (común a todos los transportes). */
    vm_buffer_init_psram();

    s_events = xEventGroupCreate();

    /* Ethernet — COMÚN: en TCP transporta el wire; en UART transporta SOLO los logs
     * (net_logf → :5555) como red de seguridad. B.5 (#138, 2-jul): NO-FATAL — si la
     * placa no tiene PHY (una P4 sin Ethernet), eth_init falla y seguimos SIN red
     * (sin logs :5555; el wire UART no depende de él). wire_task_uart ya tolera la
     * ausencia: espera Link Up con timeout de 5 s y arranca igual. */
    esp_eth_handle_t eth_handle = NULL;
    esp_err_t eth_rc = eth_init(&eth_handle);
    if (eth_rc == ESP_OK) {
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

        /* Canal de log (diagnóstico) — común a ambos transportes. */
        xTaskCreate(tcp_log_task, "tcp_log", 4096, NULL, 5, NULL);
    } else {
        ESP_LOGW(TAG, "Ethernet no disponible (%s) — sigo sin red: logs :5555 desactivados",
                 esp_err_to_name(eth_rc));
#ifdef BPVM_P4_WIRE_TCP
        ESP_LOGE(TAG, "wire TCP configurado SIN Ethernet: el IDE no podra conectar "
                      "(compila con BPVM_P4_WIRE=uart para esta placa)");
#endif
    }

#ifdef BPVM_P4_WIRE_TCP
    /* Wire por TCP. */
    xTaskCreate(wire_task, "wire_v1", 32768, NULL, 5, NULL);
    ESP_LOGI(TAG, "P4 VM.3 (TCP): IP 192.168.2.2 | log %s:%d | wire v1 TCP *:%d",
             SERVER_IP, SERVER_PORT, WIRE_PORT);
#else
    /* Wire por UART0 (COM14) + Ethernet SOLO para logs (:5555) = red de seguridad. */
    xTaskCreate(wire_task_uart, "wire_uart", 32768, NULL, 5, NULL);
    ESP_LOGI(TAG, "P4 VM.3 (UART0 + Ethernet-logs): wire por COM14, logs por :5555");
#endif

    /* G6 — la GUI la dirige BasicPlus: cuando un .mod usa Gui.*, gui.c hace lv_init()
     * + bpvm_gui_disp_init() (costura en gui_display_dsi.c) y Gui.run() bombea, todo
     * dentro del VM (wire_task). El panel/táctil arrancan en ese momento; app_main
     * solo deja red + tasks listas. (p4_gfx_lvgl_test/smoke quedan de diagnóstico.) */
}
