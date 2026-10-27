/*
 * sonido_p4.c — V8/H4 (27-oct, #708): la cintura del sonido de la P4. Lo común —leer el WAV, la tarea que lo toca, las
 * órdenes y el error de una placa sin sonido— está en la HAL BP (bpvm_sonido.c); esto sólo enciende el camino —el códec
 * ES8311 por I2S, con su amplificador—, le da PCM de 16 bits y lo apaga. Es el arranque del firmware de prueba del
 * 19-oct (C:\tmp\v8sonido\p4_wav), que sonó en tiempo real (leer la SD, el 4 %; el audio, ~7 KB de RAM interna).
 *
 * LA PLACA LO DICE EN EL ENV, con la línea `audio` (bpvm_sonido_linea_parse, en la HAL BP):
 *
 *     audio=es8311,i2c:0,mclk:13,bclk:12,ws:10,dout:9,pa:53
 *
 * Sin ella, esta placa no tiene sonido, y lo dice la HAL BP. El I2C del códec es el bus del táctil (#669, Eduardo: «Hay
 * placas donde ese I2c se utiliza también para controlar otros dispositivos»): el sonido se engancha como el firmware,
 * por el árbitro, y pone su dispositivo en el bus del IDF que abrió la cintura, como el táctil.
 *
 * AL ARRANCAR sólo se mira que el códec conteste (si no, la HAL BP da el motivo) y se reservan sus pines; el I2S y el
 * códec se preparan en el primer play. El códec y el amplificador, encendidos SÓLO MIENTRAS SUENA: esp_codec_dev los
 * enciende al abrir y los apaga al cerrar (el ahorro de energía, §E3). El volumen de 0 a 100 de esp_codec_dev son de
 * -50 a 0 dB en el códec, más +3,6 dB porque el amplificador va a 5 V y el códec a 3,3 (lo de esta placa).
 */
#include "bpvm_sonido.h"
#include "bpvm_i2c.h"
#include "bpvm_bus.h"
#include "bpvm_machine.h"
#include "hw_esp32.h"
#include "board_mgr_esp32.h"
#include "log.h"

#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bpvm_sonido_linea_t     s_l;
static i2c_master_bus_handle_t s_bus;
static esp_codec_dev_handle_t  s_codec;

/* El I2S (sólo la salida) y el códec, la primera vez que suena algo. La frecuencia y los canales los pone
 * esp_codec_dev al abrir cada WAV. */
static int p4_preparar(char* err, size_t n) {
    i2s_chan_handle_t tx = NULL;
    i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t r;
    cc.auto_clear = true;   /* si faltan datos, silencio: no lo último repetido */
    r = i2s_new_channel(&cc, &tx, NULL);
    if (r != ESP_OK) { snprintf(err, n, "el canal I2S: %s", esp_err_to_name(r)); return -1; }
    i2s_std_config_t sc = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = (gpio_num_t) s_l.mclk,
            .bclk = (gpio_num_t) s_l.bclk,
            .ws   = (gpio_num_t) s_l.ws,
            .dout = (gpio_num_t) s_l.dout,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    sc.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    r = i2s_channel_init_std_mode(tx, &sc);
    if (r != ESP_OK) {
        i2s_del_channel(tx);
        snprintf(err, n, "el I2S estandar: %s", esp_err_to_name(r));
        return -1;
    }
    /* El control por el I2C (la dirección, en 8 bits para esp_codec_dev), los datos por el I2S y el amplificador. */
    audio_codec_i2c_cfg_t ic = { .port = (uint8_t) s_l.i2c, .addr = (uint8_t) (s_l.dir << 1), .bus_handle = s_bus };
    audio_codec_i2s_cfg_t dc = { .port = I2S_NUM_0, .rx_handle = NULL, .tx_handle = tx };
    const audio_codec_ctrl_if_t* ctrl  = audio_codec_new_i2c_ctrl(&ic);
    const audio_codec_data_if_t* datos = audio_codec_new_i2s_data(&dc);
    const audio_codec_gpio_if_t* gpio  = audio_codec_new_gpio();
    if (!ctrl || !datos || !gpio) { snprintf(err, n, "las interfaces de esp_codec_dev"); return -1; }
    es8311_codec_cfg_t ec = {
        .ctrl_if     = ctrl,
        .gpio_if     = gpio,
        .codec_mode  = ESP_CODEC_DEV_WORK_MODE_DAC,
        .master_mode = false,
        .use_mclk    = true,
        .pa_pin      = (int16_t) s_l.pa,
        .pa_reverted = !s_l.pa_alto,
        .hw_gain     = { .pa_voltage = 5.0, .codec_dac_voltage = 3.3 },
        .mclk_div    = 256,
    };
    const audio_codec_if_t* es8311 = es8311_codec_new(&ec);
    if (!es8311) { snprintf(err, n, "el ES8311 no se inicia"); return -1; }
    esp_codec_dev_cfg_t dv = { .dev_type = ESP_CODEC_DEV_TYPE_OUT, .codec_if = es8311, .data_if = datos };
    s_codec = esp_codec_dev_new(&dv);
    if (!s_codec) { snprintf(err, n, "esp_codec_dev_new"); return -1; }
    return 0;
}

static int p4_abrir(uint32_t hz, int canales, char* err, size_t n) {
    if (!s_codec && p4_preparar(err, n) != 0) return -1;
    esp_codec_dev_sample_info_t si = {
        .bits_per_sample = 16,
        .channel         = (uint8_t) canales,
        .channel_mask    = (uint16_t) ((1u << canales) - 1u),
        .sample_rate     = hz,
    };
    int rc = esp_codec_dev_open(s_codec, &si);   /* enciende el códec y el amplificador */
    if (rc != ESP_CODEC_DEV_OK) { snprintf(err, n, "esp_codec_dev_open -> %d", rc); return -1; }
    return 0;
}

static int p4_escribir(const int16_t* pcm, uint32_t muestras) {   /* bloquea hasta que entra: el ritmo del sonido */
    return (esp_codec_dev_write(s_codec, (void*) pcm, (int) (muestras * sizeof(int16_t))) == ESP_CODEC_DEV_OK) ? 0 : -1;
}

static void p4_volumen(int v) { (void) esp_codec_dev_set_out_vol(s_codec, v); }

static void p4_cerrar(int drenar) {
    (void) drenar;                       /* lo encolado en el I2S son milisegundos */
    (void) esp_codec_dev_close(s_codec); /* apaga el amplificador y el códec */
}

static const bpvm_sonido_cintura_t CINTURA_ES8311 = { "es8311", p4_abrir, p4_escribir, p4_volumen, p4_cerrar };

/* El motivo, a la HAL BP (lo dirán Sound.play e isAvailable) y al log. */
static void sin_sonido(const char* fmt, ...) {
    char m[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m, sizeof m, fmt, ap);
    va_end(ap);
    bpvm_sonido_sin_cintura(m);
    log_printf("sonido: %s", m);
}

/* Al arrancar (p4_hw), después de la pantalla: con ella ya está declarado el I2C del táctil. */
void p4_sonido_registrar(void) {
    char linea[160], motivo[96], err[96];
    if (bpvm_env_get(board_mgr_env(), "audio", linea, sizeof linea) <= 0) {
        log_printf("sonido: esta placa no lo tiene (su ENV no lleva linea `audio`)");
        return;   /* el motivo de siempre, el de la HAL BP */
    }
    if (bpvm_sonido_linea_parse(linea, &s_l, motivo, sizeof motivo) != 0) {
        sin_sonido("esta placa no tiene sonido: la linea `audio` del env no vale (%s)", motivo);
        return;
    }
    if (strcmp(s_l.codec, "es8311") != 0) {
        sin_sonido("esta placa no tiene sonido: la imagen no conoce el codec '%s' de su linea `audio` (solo es8311)",
                   s_l.codec);
        return;
    }
    err[0] = 0;
    if (bpvm_i2c_abrir(s_l.i2c, s_l.sda, s_l.scl, -1, BPVM_BUS_FIRMWARE, err, sizeof err) != 0
        || (s_bus = esp32_i2c_bus_handle(s_l.i2c)) == NULL) {
        sin_sonido("esta placa no tiene sonido: el I2C %d del codec no se pudo abrir (%s)", s_l.i2c, err);
        return;
    }
    if (i2c_master_probe(s_bus, (uint16_t) s_l.dir, 100) != ESP_OK) {
        sin_sonido("esta placa no tiene sonido: el codec ES8311 no contesta en el I2C %d (0x%02X)", s_l.i2c, s_l.dir);
        return;
    }
    {   /* Sus pines, del sistema (#669, como los de la pantalla): un programa no los puede pedir. */
        const int P[] = { s_l.mclk, s_l.bclk, s_l.ws, s_l.dout, s_l.pa };
        for (size_t i = 0; i < sizeof P / sizeof P[0]; i++)
            if (P[i] >= 0 && bpvm_caps_reservar_sistema(P[i], BPVM_PIN_SIS_SONIDO, err, sizeof err) != 0)
                log_printf("sonido: el GPIO%d NO se pudo reservar: %s", P[i], err);
    }
    bpvm_sonido_registrar(&CINTURA_ES8311);
    log_printf("sonido: ES8311 en el I2C %d (0x%02X); I2S mclk %d, bclk %d, ws %d, dout %d; amplificador en el GPIO %d",
               s_l.i2c, s_l.dir, s_l.mclk, s_l.bclk, s_l.ws, s_l.dout, s_l.pa);
}
