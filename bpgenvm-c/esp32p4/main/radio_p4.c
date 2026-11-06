/*
 * radio_p4.c — V8/H3 (6-nov): LA RADIO DE LA P4, que no está en la P4. Es la de su ESP32-C6, por SDIO, con esp_hosted. La
 * WiFi en sí es la de la familia (wifi_esp32.c): esto sólo abre y cierra el enlace con la C6.
 *
 * NO TODAS LAS P4 LLEVAN C6 (Eduardo, 6-nov), y en una que no la lleve, el GPIO 54 y los de la ranura SDIO son de quien los
 * quiera. Por eso la placa lo dice en su ENV, como la `sd` y el `audio`:
 *
 *     radio=c6
 *
 * Sin esa línea no se toca ningún pin y la placa no tiene WiFi, y se dice. Las SDIO y el reset son los de compilación
 * (CONFIG_ESP_HOSTED_*): la placa de Espressif, que Waveshare copia.
 *
 * APAGADA, la C6 está EN RESET (su CHIP_PU, el GPIO 54, a bajo), desde el arranque: casi no consume (Eduardo, 19-oct: manda
 * el ahorro de energía). Al abrir el enlace, esp_hosted la saca del reset y espera a que arranque: 2,1 s con la WiFi
 * arrancada (medido el 19-oct). Como arranca de cero cada vez, `#706` no aparece.
 *
 * EL CONTROLADOR SDMMC es del sistema (Eduardo, 19-oct) y con el IDF 6.1 se comparte solo: la SD y la C6 van en ranuras
 * distintas, cada uno lo pide y suelta SÓLO su ranura. Con esp_hosted apagado, la SD funciona igual.
 *
 * UN FALLO DEL ENLACE NO REINICIA LA PLACA (CONFIG_ESP_HOSTED_HOST_TRANSPORT_RESTART_ON_FAILURE=n, que por defecto hace
 * abort()): la WiFi se queda apagada y lo dice el log.
 */
#include "wifi_esp32.h"
#include "bpvm_env.h"
#include "bpvm_machine.h"
#include "bpvm_wifi.h"
#include "board_mgr_esp32.h"
#include "log.h"

#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_hosted.h"
#include "esp_hosted_event.h"
#include "esp_timer.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static int s_eventos = 0;   /* el manejador, registrado */

/* La C6 en reset. */
static void c6_a_reset(void) {
    gpio_config_t g = {
        .pin_bit_mask = 1ULL << CONFIG_ESP_HOSTED_HOST_RESET_GPIO,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&g);
    gpio_set_level(CONFIG_ESP_HOSTED_HOST_RESET_GPIO, 0);
}

static void al_evento(void* arg, esp_event_base_t base, int32_t id, void* datos) {
    (void) arg; (void) base; (void) datos;
    if (id == ESP_HOSTED_EVENT_TRANSPORT_FAILURE) wifi_esp32_radio_rota();
}

static int abrir(char* err, size_t n) {
    static int s_version_dicha = 0;
    int64_t t0 = esp_timer_get_time();
    int ini, con;
    if (!s_eventos) {
        esp_err_t r = esp_event_handler_register(ESP_HOSTED_EVENT, ESP_EVENT_ANY_ID, al_evento, NULL);
        if (r != ESP_OK) { snprintf(err, n, "los eventos de esp_hosted: %s", esp_err_to_name(r)); return -1; }
        s_eventos = 1;
    }
    ini = esp_hosted_init();
    con = esp_hosted_connect_to_slave();
    if (ini != 0 || con != 0) {
        snprintf(err, n, "el enlace con la C6: init %d, connect_to_slave %d", ini, con);
        return -1;
    }
    log_printf("radio: el enlace con la C6, abierto en %d ms", (int) ((esp_timer_get_time() - t0) / 1000));
    if (!s_version_dicha) {   /* cada P4 necesita su C6 con esp_hosted 3.x: que se vea cuál lleva */
        esp_hosted_coprocessor_fwver_t v;
        memset(&v, 0, sizeof v);
        if (esp_hosted_get_coprocessor_fwversion(&v) == ESP_OK)
            log_printf("radio: la C6 lleva esp_hosted %" PRIu32 ".%" PRIu32 ".%" PRIu32, v.major1, v.minor1, v.patch1);
        s_version_dicha = 1;
    }
    return 0;
}

static void cerrar(void) {
    int r = esp_hosted_deinit();
    if (r != 0) log_printf("radio: esp_hosted_deinit -> %d", r);
    c6_a_reset();
}

static const wifi_esp32_enlace_t ENLACE_C6 = { abrir, cerrar };

/* Al arrancar (p4_hw). */
void p4_radio_registrar(void) {
    static const int PINES[] = {
        CONFIG_ESP_HOSTED_HOST_SDIO_PIN_CLK, CONFIG_ESP_HOSTED_HOST_SDIO_PIN_CMD,
        CONFIG_ESP_HOSTED_HOST_SDIO_PIN_D0,  CONFIG_ESP_HOSTED_HOST_SDIO_PIN_D1,
        CONFIG_ESP_HOSTED_HOST_SDIO_PIN_D2,  CONFIG_ESP_HOSTED_HOST_SDIO_PIN_D3,
        CONFIG_ESP_HOSTED_HOST_RESET_GPIO,
    };
    char v[16], err[96];
    if (bpvm_env_get(board_mgr_env(), "radio", v, sizeof v) <= 0) {
        log_printf("radio: esta placa no declara ninguna (su ENV no lleva linea `radio`)");
        return;
    }
    if (strcmp(v, "c6") != 0) {
        log_printf("radio: la imagen no conoce la radio '%s' de su linea `radio` (solo c6)", v);
        return;
    }
    for (size_t i = 0; i < sizeof PINES / sizeof PINES[0]; i++)
        if (bpvm_caps_reservar_sistema(PINES[i], BPVM_PIN_SIS_RADIO, err, sizeof err) != 0)
            log_printf("radio: el GPIO%d NO se pudo reservar: %s", PINES[i], err);
    c6_a_reset();
    wifi_esp32_publicar(&ENLACE_C6);
    log_printf("radio: la C6 por SDIO (ranura %d), en reset hasta que haga falta", CONFIG_ESP_HOSTED_HOST_SDIO_SLOT);
}
