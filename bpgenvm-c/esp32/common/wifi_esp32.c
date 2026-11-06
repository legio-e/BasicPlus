/*
 * wifi_esp32.c — V8/H3 (6-nov): la radio de la familia ESP. Publica 'WIFI' (el contrato, en bpvm_wifi.h): encender,
 * conectar, apagar y la señal. Los reintentos, los estados y cuándo apagarla son de la HAL BP: aquí sólo se traduce a
 * `esp_wifi_*`. Es la secuencia del firmware de prueba del 19-oct (C:\tmp\v8wifi\p4_wifi), que hizo tres vueltas sin fallo.
 *
 * APAGADA DEL TODO es `esp_wifi_stop` + `esp_wifi_deinit`, y en la P4 además cerrar el enlace con la C6 (Eduardo, 19-oct:
 * manda el ahorro de energía): la RAM vuelve, y al encender la C6 arranca de cero.
 *
 * La configuración va en RAM (WIFI_STORAGE_RAM): la red la dice /sys/wifi.json, y la radio no tiene por qué escribir nada
 * en la flash en cada conexión.
 */
#include "wifi_esp32.h"
#include "net_esp32.h"
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "bpvm_wifi.h"
#include "log.h"

#include "esp_event.h"
#include "esp_wifi.h"

#include <stdio.h>
#include <string.h>

static const wifi_esp32_enlace_t* s_enlace = NULL;
static bpvm_wifi_aviso_fn s_aviso = NULL;
static int s_eventos = 0;       /* el manejador, registrado */

static int clase_de(int motivo) {
    switch (motivo) {
    case WIFI_REASON_AUTH_FAIL:
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_MIC_FAILURE:
        return BPVM_WIFI_AV_CLAVE;
    case WIFI_REASON_NO_AP_FOUND:
    case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
    case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
    case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
        return BPVM_WIFI_AV_SIN_RED;
    default:
        return BPVM_WIFI_AV_CAIDA;
    }
}

/* En la tarea de eventos del IDF: lo justo. */
static void al_evento(void* arg, esp_event_base_t base, int32_t id, void* datos) {
    (void) arg; (void) base;
    if (!s_aviso) return;
    if (id == WIFI_EVENT_STA_CONNECTED) {
        s_aviso(BPVM_WIFI_AV_CONECTADA, 0);
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t* d = (const wifi_event_sta_disconnected_t*) datos;
        s_aviso(clase_de(d->reason), d->reason);
    }
}

void wifi_esp32_radio_rota(void) {
    if (s_aviso) s_aviso(BPVM_WIFI_AV_ROTA, 0);
}

static int fallo(char* err, size_t n, const char* que, esp_err_t r) {
    snprintf(err, n, "%s: %s", que, esp_err_to_name(r));
    return -1;
}

static int r_encender(const char* pais, bpvm_wifi_aviso_fn aviso, char* err, size_t n) {
    esp_err_t r;
    s_aviso = aviso;
    if (net_esp32_preparar(err, n) != 0) return -1;
    if (!s_eventos) {
        r = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, al_evento, NULL);
        if (r != ESP_OK) return fallo(err, n, "los eventos de la WiFi", r);
        s_eventos = 1;
    }
    if (s_enlace && s_enlace->abrir(err, n) != 0) return -1;
    {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        r = esp_wifi_init(&cfg);
    }
    if (r != ESP_OK) return fallo(err, n, "esp_wifi_init", r);
    r = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (r != ESP_OK) log_printf("wifi: esp_wifi_set_storage: %s", esp_err_to_name(r));
    r = esp_wifi_set_mode(WIFI_MODE_STA);
    if (r != ESP_OK) return fallo(err, n, "esp_wifi_set_mode", r);
    if (pais[0]) {
        r = esp_wifi_set_country_code(pais, true);
        if (r != ESP_OK) log_printf("wifi: el pais %s no se pudo poner: %s", pais, esp_err_to_name(r));
    }
    r = esp_wifi_start();
    if (r != ESP_OK) return fallo(err, n, "esp_wifi_start", r);
    return 0;
}

static void r_apagar(void) {
    (void) esp_wifi_stop();      /* sin iniciar, dicen que no: da igual, vale tras un encender a medias */
    (void) esp_wifi_deinit();
    if (s_enlace) s_enlace->cerrar();
}

static int r_conectar(const char* ssid, const char* clave, char* err, size_t n) {
    wifi_config_t wc;
    esp_err_t r;
    size_t ls = strlen(ssid), lc = strlen(clave);
    memset(&wc, 0, sizeof wc);
    /* Sin el NUL al final si llena el campo: 32 y 64 valen enteros. */
    memcpy(wc.sta.ssid, ssid, ls > sizeof wc.sta.ssid ? sizeof wc.sta.ssid : ls);
    memcpy(wc.sta.password, clave, lc > sizeof wc.sta.password ? sizeof wc.sta.password : lc);
    wc.sta.threshold.authmode = WIFI_AUTH_OPEN;   /* la que tenga la red: la seguridad la dice ella */
    wc.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    r = esp_wifi_set_config(WIFI_IF_STA, &wc);
    if (r != ESP_OK) return fallo(err, n, "esp_wifi_set_config", r);
    r = esp_wifi_connect();
    if (r != ESP_OK) return fallo(err, n, "esp_wifi_connect", r);
    return 0;
}

static int r_rssi(void) {
    wifi_ap_record_t ap;
    return esp_wifi_sta_get_ap_info(&ap) == ESP_OK ? ap.rssi : 0;
}

static const bpvm_pack_sym_t SIMBOLOS[] = {
    { "encender", (bpvm_pack_fn_t) r_encender },
    { "apagar",   (bpvm_pack_fn_t) r_apagar },
    { "conectar", (bpvm_pack_fn_t) r_conectar },
    { "rssi",     (bpvm_pack_fn_t) r_rssi },
};
static const bpvm_pack_api_t TABLA = {
    BPVM_WIFI_MARCA, BPVM_WIFI_API_VER, sizeof SIMBOLOS / sizeof SIMBOLOS[0], SIMBOLOS
};

void wifi_esp32_publicar(const wifi_esp32_enlace_t* enlace) {
    int r;
    s_enlace = enlace;
    net_esp32_registrar();
    r = bpvm_bios_publica(BPVM_WIFI_MARCA, &TABLA);
    if (r != 0) log_printf("wifi: la radio no se pudo publicar (%d)", r);
}
