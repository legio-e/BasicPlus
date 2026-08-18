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
#include "board_mgr_esp32.h"   /* H9: arranque escalonado + estado del boot */
#include "log.h"               /* log persistente (post-mortem) — lo antes posible */
#include "bpvm.h"              /* #353: bpvm_diag_set_sink — el rastro de la VM al log */
#include "bpvm_sqlmem.h"       /* V5/H7: la REGLA del bloque de la BD (un solo sitio) */
#include "bpvm_env.h"          /* V5/H7: SQLite=<MB> del entorno */
#include "bpvm_bios.h"         /* V5/H7: la tabla que se le presta al pack */
#include "pack_p4.h"           /* V5/H7: PACK_RAM_BYTES + s_pack_ram_base + bios_p4_get */
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
#include "bpvm_blk_sdmmc.h"   /* V5/H6: la SD por SDIO como dispositivo de bloque */
#include "blk_sdmmc_p4.h"     /* SDIO_KHZ_POR_DEFECTO: para ANUNCIAR el reloj de verdad */
#include "bpvm_fs_fat.h"      /* V5/H6: montar esa tarjeta como FAT bajo /sd      */

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
/* H9 — reparto de la PSRAM (Eduardo 19-jul): ~4 MB reservados para el framebuffer
 * del display (LVGL reserva ON-DEMAND, más tarde, cuando un .mod usa Gui.*), y
 * TODO EL RESTO para el heap de la VM. Se calcula en runtime (free PSRAM − reserva)
 * → auto-adapta a la PSRAM real (16/32 MB) sin hornear el tamaño. Antes: 2 MiB fijos. */
#define VM_PSRAM_DISPLAY_RESERVE  (4u * 1024u * 1024u)   /* margen para LVGL */
#define VM_MEM_MIN                (2u * 1024u * 1024u)    /* piso si algo va mal */
uint8_t*       s_vm_buffer      = NULL;
uint32_t       s_vm_buffer_size = 0;

/* V5/H7 — el bloque de la BD (SQLite), mordido de la PSRAM ANTES que el heap de
 * la VM. Los lee `bios_p4.c` para servir la arena. NULL/0 = no se pidió o no cupo. */
uint8_t*       s_sqlite_base    = NULL;
uint32_t       s_sqlite_size    = 0;
static int     s_sqlite_res     = 0;      /* bpvm_sqlite_res_t, para el INFO */
static long    s_sqlite_asked_mb = 0;

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

/* ===================================================================
 * CANAL DE LOG POR RED (net_logf → 192.168.2.1:5555) — APAGADO EN V4
 * ===================================================================
 * Viene del bring-up del P4, cuando el enlace con el IDE era por Ethernet.
 * Después se añadió el transporte por UART0 (#138) para que UNA MISMA IMAGEN
 * sirviera a las dos placas P4 —la del kit, con Ethernet, y la Waveshare, que no
 * lo tiene— y desde entonces la red aquí sólo alimentaba ese canal de
 * diagnóstico. Es historia, no una función del producto.
 *
 * Lo que costaba en la imagen que se publica (transporte UART, el de por
 * defecto): `wire_task_uart` esperaba hasta CINCO SEGUNDOS a que subiera el
 * enlace para que esos logs llegaran. En la Waveshare, sin PHY, ese enlace no
 * sube NUNCA → cinco segundos de retraso en CADA encendido, esperando un canal
 * que nadie escucha.
 *
 * Se apaga con un interruptor y NO se borra: publicamos las fuentes, así que
 * quien lo quiera compila con -DBPVM_P4_NETLOG=1 y lo tiene entero. Se prefirió
 * el interruptor a comentar las líneas por una razón concreta: comentando sólo
 * las llamadas quedan cuatro funciones estáticas sin usar, y este proyecto
 * compila con -Werror → no enlazaría. Así el código sigue vivo y compilable.
 *
 * OJO: el wire por TCP (BPVM_P4_WIRE=tcp) SÍ necesita la red. Hay un #error
 * abajo que lo dice en vez de dejarte un binario que no conecta.
 * =================================================================== */
#ifndef BPVM_P4_NETLOG
#define BPVM_P4_NETLOG 0
#endif
#if defined(BPVM_P4_WIRE_TCP) && !BPVM_P4_NETLOG
#error "BPVM_P4_WIRE=tcp necesita la red: compila tambien con -DBPVM_P4_NETLOG=1"
#endif

#if BPVM_P4_NETLOG
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

#endif  /* BPVM_P4_NETLOG */

/* ---------------------------------------------------------------------------
 * V5/H6 — la tarjeta SD, si el env la configura.
 *
 * AQUI y no antes: /sd se monta ENCIMA de la fachada, y sin el FS de la placa no
 * hay donde colgarlo. Y antes del autoarranque, para que una app encuentre la
 * tarjeta ya montada.
 *
 * Lo llaman los DOS caminos de arranque (TCP y UART) desde una sola funcion, que
 * es lo que evita el clasico "lo arregle en uno y me olvide del otro".
 *
 * (!) A diferencia de la Metro, el zocalo del P4 NO tiene deteccion de tarjeta
 * (~CD solo lleva su pull-up, leido del esquema), asi que NO se usa
 * `bpvm_fs_fat_vigilar`: esa funcion se rearma en la rama "no hay medio", que
 * aqui no se ejecutaria nunca, y quedaria dando UN intento por arranque sin
 * decir por que. En esta placa el montaje es explicito.
 *
 * (!) Que falle NO degrada el arranque. Una tarjeta ilegible, ausente o en exFAT
 * es un accesorio que no va, no una placa rota: se anota el motivo y se sigue.
 * Degradar el boot por esto dejaria al usuario sin IDE justo cuando mas falta
 * hace para averiguar que pasa.
 *
 * (!) Va por `log_printf` y NO por `net_logf`: el segundo escribe a la consola
 * del IDF y al socket de log por red, que esta APAGADO por defecto. El log que
 * de verdad se mira -el persistente, el que sobrevive a un cuelgue y el que
 * enseña el IDE- es el de `log_printf`, y es donde estan las demas lineas del
 * arranque. Un chivato en el canal equivocado es un chivato mudo.
 * ------------------------------------------------------------------------- */
static void p4_montar_sd(void)
{
    char linea[160];
    int n = bpvm_env_get(board_mgr_env(), "sd", linea, sizeof linea);
    if (n <= 0) { log_printf("sd: sin configurar en el env - no se monta"); return; }

    bpvm_sdio_pines_t pines;
    char motivo[120];
    if (bpvm_sdio_pines_parse(linea, &pines, motivo, sizeof motivo) != 0) {
        log_printf("sd: %s", motivo);         /* el parser ya dice QUE falta */
        return;
    }
    /* La CONFIGURACION que de verdad ha llegado al driver, ANTES de intentar
     * nada. Esta linea existe por una tarde entera de rondas preguntandonos dos
     * cosas que el log no contestaba: "¿la imagen que corre es la nueva?" y
     * "¿el env llego hasta aqui?". Ahora las dos se leen de un vistazo, y ADEMAS
     * cambia cuando cambia el codigo, que es justo lo que el sello de build no
     * hace. Un chivato de configuracion vale mas que uno de resultado. */
    /* ⚠️ EL RELOJ, EL EFECTIVO. Antes se imprimía `pines.khz`, que es lo que
     * PIDE el env — y con el env vacío eso sale «0 kHz», que no es ninguna
     * velocidad. Costó dar por buena una prueba del bus sin saber a qué
     * frecuencia se había hecho (15-ago): 2 MB ida y vuelta sin una diferencia,
     * y el log diciendo «0».
     *
     * El driver resuelve `khz > 0 ? khz : SDIO_KHZ_POR_DEFECTO`, así que aquí se
     * hace la misma cuenta y se dice de DÓNDE sale. Un chivato de configuración
     * que declara un valor imposible es peor que no tenerlo: éste es el fichero
     * donde se decide a qué velocidad va el bus, y esa decisión —dice su propio
     * comentario— «se toma MIDIENDO, no por optimismo». Sin este dato no se
     * puede anotar la medida. */
    log_printf("sd: SDIO slot %d, %d bit(s), clk %d cmd %d d0 %d | pwr %d (activo %s) | ldo %d | %d kHz (%s)",
               pines.slot, pines.ancho, pines.clk, pines.cmd, pines.d0,
               pines.pwr, pines.pwr_activo_alto ? "alto" : "bajo",
               pines.ldo,
               pines.khz > 0 ? pines.khz : SDIO_KHZ_POR_DEFECTO,
               pines.khz > 0 ? "del env" : "por defecto");

    if (bpvm_fs_fat_montar(bpvm_blk_sdmmc(&pines), "/sd", motivo, sizeof motivo) == 0) {
        log_printf("sd: montada en /sd (%d bits, particion en el bloque %u)",
                   pines.ancho, (unsigned) bpvm_fs_fat_lba_particion());
    } else {
        log_printf("sd: NO montada - %s", motivo);
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

    /* H9 — arranque escalonado (particiones del env → FS → VM). Sin FS el climb
     * se queda abajo y el host conduce; stdlib solo con el FS montado. */
    board_mgr_esp32_boot();
    const bpvm_boot_status_t* bs = board_boot_status();
    if (bs->state >= BPVM_BOOT_FS) { fs_register_bpvm(); esp32_mods_install(); }
    if (bs->state >= BPVM_BOOT_FS) p4_montar_sd();   /* V5/H6 */
    /* V5/H7 — mapear la zona de packs y DECIR sus direcciones. Se hace
     * aunque no haya pack grabado: con `0 candidatos` el log ya deja el
     * dato que el IDE necesita para SELLAR uno (la direccion la asigna la
     * MMU en runtime, o sea que no se puede saber desde el PC). */
    /* V5/H7 (16-ago) - el arranque MAPEA la zona (barato, y el IDE la necesita
     * ya); BUSCAR el ancla y SALTAR se hace en el primer `Run`, como en la
     * Pico. Antes aquí se hacía todo: 338 ms de cada arranque, y el único paso
     * que puede colgar puesto justo donde un cuelgue obliga a regrabar. */
    if (bs->state >= BPVM_BOOT_FS) {
        (void) pack_p4_mapear();
        repl_set_packs_loader(pack_p4_cargar);
    }
    net_logf("[p4] boot estado %d (%s)%s, %d ficheros",
             (int) bs->state, bpvm_boot_state_name(bs->state),
             bs->degraded ? " DEGRADADO" : "", fs_file_count());

    esp32_hw_register();         /* H14: backends GPIO/UART/SPI/I2C (reúso del S3, ESP-IDF) */
    p4_install_board_id();       /* INFO/HELLO esp32p4 + Pico.* backend del P4 (pisa el del S3) */
    wire_v1_tcp_server_init(WIRE_PORT);
    net_logf("[p4] VM.3: esperando al IDE en *:%d (backend 'VM (TCP v1)' -> 192.168.2.2:%d)",
             WIRE_PORT, WIRE_PORT);

    /* bucle del REPL (accept/reconnect en wire_v1_recv_line). H9: autorun solo
     * con la placa sana en estado 3. */
    if (bs->state == BPVM_BOOT_APP && !bs->degraded) repl_esp32_autorun();
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
#if BPVM_P4_NETLOG
    xEventGroupWaitBits(s_events, LINK_UP_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
#endif

    board_mgr_esp32_boot();
    const bpvm_boot_status_t* bs = board_boot_status();
    if (bs->state >= BPVM_BOOT_FS) { fs_register_bpvm(); esp32_mods_install(); }
    if (bs->state >= BPVM_BOOT_FS) p4_montar_sd();   /* V5/H6 */
    /* V5/H7 — mapear la zona de packs y DECIR sus direcciones. Se hace
     * aunque no haya pack grabado: con `0 candidatos` el log ya deja el
     * dato que el IDE necesita para SELLAR uno (la direccion la asigna la
     * MMU en runtime, o sea que no se puede saber desde el PC). */
    /* V5/H7 (16-ago) - el arranque MAPEA la zona (barato, y el IDE la necesita
     * ya); BUSCAR el ancla y SALTAR se hace en el primer `Run`, como en la
     * Pico. Antes aquí se hacía todo: 338 ms de cada arranque, y el único paso
     * que puede colgar puesto justo donde un cuelgue obliga a regrabar. */
    if (bs->state >= BPVM_BOOT_FS) {
        (void) pack_p4_mapear();
        repl_set_packs_loader(pack_p4_cargar);
    }
    net_logf("[p4] boot estado %d (%s)%s (UART), %d ficheros",
             (int) bs->state, bpvm_boot_state_name(bs->state),
             bs->degraded ? " DEGRADADO" : "", fs_file_count());

    esp32_hw_register();         /* H14: backends GPIO/UART/SPI/I2C (reúso del S3) */
    p4_install_board_id();       /* INFO/HELLO esp32p4 + Pico.* del P4 */
    wire_v1_uart_init();
    net_logf("[p4] VM.3 (UART0): wire v1 por el bridge USB-UART; conecta el IDE");

    /* #423 — A PARTIR DE AQUI, EL LOG LO MANDA EL ENTORNO (`log=0|1`).
     *
     * El arranque entero queda registrado SIEMPRE: son unas quince lineas y no
     * llenan nada, y son justo las que hacen falta cuando una placa no arranca.
     * Lo que se apaga es el rastro de EJECUCION — el que llena la region de
     * 8 KB en ~26 colectas del GC y hacia que un cuelgue no dejara su ultimo
     * momento escrito (#423).
     *
     * Por defecto APAGADO: se enciende con `log=1` en el entorno cuando se va a
     * depurar, que es lo que pidio Eduardo. Para moverlo mas arriba, basta con
     * subir esta llamada: todo lo que quede por encima se registra siempre. */
    bpvm_log_set_enabled(bpvm_env_get_bool(board_mgr_env(), "log", 0));

    if (bs->state == BPVM_BOOT_APP && !bs->degraded) repl_esp32_autorun();  /* H9 */
    repl_esp32_run();            /* no retorna */
}

#endif  /* BPVM_P4_WIRE_TCP */

/* Reserva el heap de la VM en PSRAM. El P4 SIEMPRE lleva PSRAM (in-package, la
 * comparte con el framebuffer del display); si la reserva falla el firmware no es
 * operativo → abort con mensaje claro (mejor que arrancar y colgar luego en el
 * primer programa grande). Los firmwares sin PSRAM (S3/Pico/STM32) mantienen su
 * buffer en SRAM; esto es board-specific del P4. */
/*
 * V5/H7 — EL BLOQUE DE LA BD, Y MUERDE PRIMERO.
 *
 * Va ANTES de `vm_buffer_init_psram()` y no es una preferencia: es lo que hace
 * que su dirección sea DETERMINISTA. El pack nativo de SQLite se realoja en el
 * PC para una dirección concreta y lleva ese SELLO dentro (`linked_ram`); si el
 * bloque saliera de lo que quedara libre, lo daría el alocador y podría cambiar
 * entre arranques — y un pack ya grabado funcionaría una vez y luego no, sin
 * patrón. Es la misma regla que la Metro: *la BD muerde primero*.
 *
 * El tamaño sale del env (`SQLite=<MB>`) y la regla de cuánto se reserva vive en
 * `bpvm_sqlmem` — un solo sitio para el arranque, el INFO y el IDE. Por eso hay
 * que leer el env ANTES, con `board_mgr_esp32_env_temprano()`: el arranque
 * escalonado corre mucho después (ver el porqué largo en `board_mgr_esp32.c`).
 *
 * No reservar NO es un fallo: es la respuesta correcta si nadie pidió BD. Pero
 * se dice en el log SIEMPRE y con el motivo, porque "no se pidió", "se pidió
 * poco" y "no cupo" mandan a sitios distintos.
 */
static void vm_sqlite_init_psram(void)
{
    board_mgr_esp32_env_temprano();
    s_sqlite_asked_mb = bpvm_env_get_long(board_mgr_env(), "SQLite", 0);

    size_t freeps  = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t sqlbytes = 0;
    bpvm_sqlite_res_t r = bpvm_sqlite_region(s_sqlite_asked_mb, freeps, &sqlbytes);
    s_sqlite_res = (int) r;

    if (r != BPVM_SQLITE_OK) {
        log_printf("bd: SIN bloque — %s (SQLite=%ld en el ENV, PSRAM libre %u KB)",
                   bpvm_sqlite_res_str(r), s_sqlite_asked_mb,
                   (unsigned) (freeps / 1024u));
        return;
    }

    /*
     * ALINEADO A PÁGINA, y las dos razones son distintas:
     *
     * 1. CORRECCIÓN. Las secciones `.data`/`.sdata` del pack piden alineación 8
     *    (lo dice su ELF) y ahí viven estructuras de SQLite con `double` y
     *    enteros de 64 bits. `heap_caps_malloc` devuelve alineado a 4: el primer
     *    arranque dio 0x48000a7c, que NO es múltiplo de 8. El Pico ya lo hacía
     *    (`& ~7u`, "porque ahí van estructuras del pack") y al portarlo se me
     *    quedó fuera.
     *
     * 2. ESTABILIDAD DEL SELLO, que es lo que de verdad importa. Ese 0xa7c de
     *    desplazamiento es de lo que la IDF reservara en PSRAM ANTES que
     *    nosotros; si un día eso crece o mengua, nuestra dirección se mueve y el
     *    pack grabado deja de valer. Pidiendo página (4 KB), el alocador
     *    redondea hacia arriba y una variación pequeña de lo que hay delante
     *    YA NO NOS MUEVE — sólo saltaría de página si el cambio pasara de 4 KB.
     *    No es una garantía absoluta (no existe con un alocador de por medio),
     *    pero convierte "frágil ante cualquier byte" en "estable salvo cambio
     *    grande". Y si aun así se moviera, el SELLO lo caza y lo dice.
     */
    s_sqlite_base = (uint8_t*) heap_caps_aligned_alloc(4096, sqlbytes, MALLOC_CAP_SPIRAM);
    if (s_sqlite_base == NULL) {
        /* Pedirlo y no conseguirlo NO se traga: la regla dijo que cabía y el
         * alocador dice que no, y esa discrepancia hay que verla. La VM sigue
         * arrancando (sin BD), que es mejor que abortar por una función opcional. */
        s_sqlite_res = (int) BPVM_SQLITE_NO_CABE;
        log_printf("bd: la regla daba %u KB pero heap_caps_malloc DIJO QUE NO "
                   "(PSRAM libre %u KB) — se sigue SIN BD",
                   (unsigned) (sqlbytes / 1024u), (unsigned) (freeps / 1024u));
        return;
    }
    s_sqlite_size   = (uint32_t) sqlbytes;
    /* La RAM del pack sale del PRINCIPIO de este bloque: `[estáticos | arena]`. */
    s_pack_ram_base = s_sqlite_base;

    log_printf("bd: bloque %u KB @%p (SQLite=%ld MB) | pack %u B delante, arena %u KB",
               (unsigned) (sqlbytes / 1024u), (void*) s_sqlite_base, s_sqlite_asked_mb,
               (unsigned) PACK_RAM_BYTES,
               (unsigned) ((sqlbytes - PACK_RAM_BYTES) / 1024u));
}

static void vm_buffer_init_psram(void)
{
    /* Toda la PSRAM libre MENOS la reserva del display (que LVGL alocará luego,
     * on-demand, dentro del VM). Reintenta bajando 1 MiB si la mayor no cabe por
     * fragmentación, hasta un piso. */
    size_t freeps = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t want = (freeps > VM_PSRAM_DISPLAY_RESERVE)
                  ? (freeps - VM_PSRAM_DISPLAY_RESERVE) : 0u;
    want &= ~((size_t) 4095u);   /* alineado al sector */
    while (want >= VM_MEM_MIN) {
        s_vm_buffer = (uint8_t*) heap_caps_malloc(want, MALLOC_CAP_SPIRAM);
        if (s_vm_buffer != NULL) break;
        want -= 1024u * 1024u;
    }
    if (s_vm_buffer == NULL) {
        ESP_LOGE(TAG, "PSRAM: no se pudo reservar el heap de la VM (free=%u KiB) - abort",
                 (unsigned)(freeps / 1024u));
        abort();
    }
    s_vm_buffer_size = (uint32_t) want;
    ESP_LOGI(TAG, "VM heap en PSRAM: %u KiB (reserva display %u KiB, PSRAM libre %u KiB) @ %p",
             (unsigned)(want / 1024u), (unsigned)(VM_PSRAM_DISPLAY_RESERVE / 1024u),
             (unsigned)(freeps / 1024u), (void*) s_vm_buffer);
    /* #329 — al log PERSISTENTE también, con el mismo formato que el S3: la
     * consola se pierde al desconectar y las dos placas tienen que contar lo
     * mismo para poder compararlas (que es justo lo que pide #328). Aquí además
     * interesa la DRAM INTERNA, no la PSRAM: el heap de la VM vive fuera, pero
     * littlefs y el wire siguen tirando de la interna. */
    log_printf("vm: heap %u KB en PSRAM @%p (PSRAM libre %u KB) | DRAM interna libre %u B (bloque mayor %u B)",
               (unsigned)(want / 1024u), (void*) s_vm_buffer,
               (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024u),
               (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
               (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
}

/* #353 — sink de diagnóstico de la VM: al log persistente.
 *
 * ⚠️ Esto FALTABA en el P4, y era la única familia sin ponerlo — el S3 lo tiene
 * en su main.c y el STM32 en su repl. Consecuencia, medida el 15-ago con
 * Eduardo: en esta placa había log de ARRANQUE (lo escribe este fichero con
 * `log_printf`) pero NINGUNO de ejecución. Todo lo que dice la VM mientras corre
 * un programa —de dónde salió cada módulo, qué dependencia no encontró, el
 * veredicto del guardián de fin de RUN— se iba al `stderr` por defecto, que aquí
 * es la consola USB-JTAG: un sitio que nadie mira y que además no sobrevive al
 * reset.
 *
 * El coste real de no tenerlo: una mañana entera de hipótesis sobre por qué un
 * `Core.mod` en `/lib` daba «IO error», con el firmware sabiendo la respuesta y
 * contándosela a nadie. */
static void diag_al_log(const char* linea) { log_printf("%s", linea); }

void app_main(void)
{
    /* LO PRIMERO: el log persistente. Recupera el snapshot de la sesión anterior
     * (post-mortem: si el arranque previo se fue al garete, aquí está escrito) y
     * queda grabando desde antes de la PSRAM y del climb del boot — vive en RAM
     * interna + un sector libre de bpenv, así que no depende de ninguno. */
    log_init();
    bpvm_diag_set_sink(diag_al_log);           /* #353 — FALTABA en esta familia */
    log_printf("=== boot ESP32-P4 ===");
    /* #439 — DE DONDE sale lo que trae el log. Sin esto, un volcado con dos
     * arranques no distingue «la RAM sobrevivio» de «se cargo de flash lo que
     * el arranque anterior volco» — que es exactamente lo que habia antes.
     * Estaba solo en la Pico: el mecanismo viajo a las 4 imagenes y el AVISO
     * no. Un arreglo a medias es el que no se puede comprobar. */
    log_printf(bpvm_log_origen_ram()
               ? "log: RAM SUPERVIVIENTE (lineas de ANTES del reset)"
               : "log: arranque en frio (cargado de flash)");


    /* V5/H7 — el bloque de la BD muerde ANTES que el heap de la VM: su dirección
     * va SELLADA dentro del pack nativo y tiene que ser la misma cada arranque.
     * Ver el porqué en vm_sqlite_init_psram(). */
    vm_sqlite_init_psram();

    /* Heap de la VM en PSRAM (común a todos los transportes), con lo que quede. */
    vm_buffer_init_psram();

    /* La tabla BIOS se verifica SIEMPRE, haya pack o no: así un hueco se
     * descubre hoy y no el día que alguien grabe uno. El que la usa es el pack,
     * por el ancla; esto sólo comprueba.
     *
     * Y DICE QUE SÍ, no sólo que no. `bios_p4_get()` ya escribe cuál ranura
     * falta cuando falla, pero callar al ir bien deja un silencio ambiguo: no
     * distingue "completa" de "esta línea no se ejecutó". En un arranque nuevo
     * eso es justo lo que hay que poder distinguir. */
    log_printf("bios: tabla %s (%d ranuras)",
               bios_p4_get() ? "COMPLETA" : "INCOMPLETA", bpvm_bios_slot_count());

    s_events = xEventGroupCreate();

#if BPVM_P4_NETLOG
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
#endif  /* BPVM_P4_NETLOG */

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
