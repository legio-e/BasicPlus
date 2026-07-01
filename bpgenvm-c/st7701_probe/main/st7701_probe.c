// st7701_probe.c — Sonda mínima del panel ST7701 (Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, 480x800).
//
// Objetivo: KNOWN-GOOD del display SIN LVGL / SIN BSP de Waveshare / SIN la VM de BasicPlus.
//   backlight ON (LEDC invertido) -> LDO DPHY -> bus DSI 2-lane -> panel ST7701 con su tabla
//   de init DCS -> barras de color con el generador de patrón del DPI.
// Si se ven las barras, el panel + los timings + el init están bien y podemos portar esto a
// gui_display_dsi.c con confianza.
//
// Consola por UART0 => los logs salen por COM15 (la placa NO tiene Ethernet: es nuestra unica
// ventana de observabilidad durante el bring-up).
//
// Datos (IC, resolucion, timings, pines, LDO y la tabla de init) extraidos LITERALMENTE del BSP
// de Waveshare (componente espressif/esp_lcd_st7701 + esp32_p4_wifi6_touch_lcd_4_3), que NO
// compila en IDF v6 por arrastrar LVGL 9.4; aqui usamos solo el driver del panel, que si va.

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/ledc.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7701.h"

static const char *TAG = "st7701_probe";

// ---- Pines / parametros de la placa (del BSP Waveshare 4.3") ----
#define PIN_BL        26    // backlight, LEDC con logica INVERTIDA (duty 100% => encendido)
#define PIN_RST       27    // reset del panel
#define LCD_H_RES     480
#define LCD_V_RES     800
#define DSI_LANES     2
#define DSI_MBPS      500   // (la Function-EV usa 1000; esta placa 500)
#define DPI_CLK_MHZ   30    // (la Function-EV usa 52)
#define LDO_CHAN      3     // LDO_VO3 -> VDD_MIPI_DPHY
#define LDO_MV        2500

// Tabla de init DCS especifica del panel — COPIADA LITERAL del BSP de Waveshare.
// Formato: { comando, {parametros...}, num_parametros, delay_ms }.
static const st7701_lcd_init_cmd_t vendor_init_cmds[] = {
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t[]){0x08}, 1, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]){0x63, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t[]){0x17, 0x08}, 2, 0},
    {0xCC, (uint8_t[]){0x10}, 1, 0},
    {0xB0, (uint8_t[]){0x40, 0xC9, 0x94, 0x0E, 0x10, 0x05, 0x0B, 0x09, 0x08, 0x26, 0x04, 0x52, 0x10, 0x69, 0x6B, 0x69}, 16, 0},
    {0xB1, (uint8_t[]){0x40, 0xD2, 0x98, 0x0C, 0x92, 0x07, 0x09, 0x08, 0x07, 0x25, 0x02, 0x0E, 0x0C, 0x6E, 0x78, 0x55}, 16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t[]){0x5D}, 1, 0},
    {0xB1, (uint8_t[]){0x4E}, 1, 0},
    {0xB2, (uint8_t[]){0x87}, 1, 0},
    {0xB3, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x4E}, 1, 0},
    {0xB7, (uint8_t[]){0x85}, 1, 0},
    {0xB8, (uint8_t[]){0x21}, 1, 0},
    {0xB9, (uint8_t[]){0x10, 0x1F}, 2, 0},
    {0xBB, (uint8_t[]){0x03}, 1, 0},
    {0xBC, (uint8_t[]){0x00}, 1, 0},
    {0xC1, (uint8_t[]){0x78}, 1, 0},
    {0xC2, (uint8_t[]){0x78}, 1, 0},
    {0xD0, (uint8_t[]){0x88}, 1, 0},
    {0xE0, (uint8_t[]){0x00, 0x3A, 0x02}, 3, 0},
    {0xE1, (uint8_t[]){0x04, 0xA0, 0x00, 0xA0, 0x05, 0xA0, 0x00, 0xA0, 0x00, 0x40, 0x40}, 11, 0},
    {0xE2, (uint8_t[]){0x30, 0x00, 0x40, 0x40, 0x32, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00}, 13, 0},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE4, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t[]){0x09, 0x2E, 0xA0, 0xA0, 0x0B, 0x30, 0xA0, 0xA0, 0x05, 0x2A, 0xA0, 0xA0, 0x07, 0x2C, 0xA0, 0xA0}, 16, 0},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE7, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t[]){0x08, 0x2D, 0xA0, 0xA0, 0x0A, 0x2F, 0xA0, 0xA0, 0x04, 0x29, 0xA0, 0xA0, 0x06, 0x2B, 0xA0, 0xA0}, 16, 0},
    {0xEB, (uint8_t[]){0x00, 0x00, 0x4E, 0x4E, 0x00, 0x00, 0x00}, 7, 0},
    {0xEC, (uint8_t[]){0x08, 0x01}, 2, 0},
    {0xED, (uint8_t[]){0xB0, 0x2B, 0x98, 0xA4, 0x56, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x65, 0x4A, 0x89, 0xB2, 0x0B}, 16, 0},
    {0xEF, (uint8_t[]){0x08, 0x08, 0x08, 0x45, 0x3F, 0x54}, 6, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x11, (uint8_t[]){0x00}, 0, 120},   // Sleep Out + 120 ms
    {0x29, (uint8_t[]){0x00}, 0, 0},     // Display On
};

// Enciende el backlight por PWM (LEDC). OJO: output_invert=1 en esta placa.
static void backlight_init_on(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,   // 0..1023
        .timer_num       = LEDC_TIMER_1,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .gpio_num   = PIN_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 1023,   // 100% -> con output_invert => backlight ENCENDIDO
        .hpoint     = 0,
        .flags.output_invert = 1,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));
    ESP_LOGI(TAG, "Backlight ON (GPIO%d, LEDC invertido, duty=1023)", PIN_BL);
}

static esp_lcd_panel_handle_t panel_init(void)
{
    // 1) LDO interno para alimentar el PHY MIPI-DSI.
    esp_ldo_channel_handle_t ldo = NULL;
    esp_ldo_channel_config_t ldo_cfg = { .chan_id = LDO_CHAN, .voltage_mv = LDO_MV };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &ldo));
    ESP_LOGI(TAG, "LDO DPHY ch%d @%d mV OK", LDO_CHAN, LDO_MV);

    // 2) Bus MIPI-DSI (inicializa tambien el PHY).
    esp_lcd_dsi_bus_handle_t dsi = NULL;
    esp_lcd_dsi_bus_config_t bus = {
        .bus_id             = 0,
        .num_data_lanes     = DSI_LANES,
        .phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = DSI_MBPS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus, &dsi));
    ESP_LOGI(TAG, "Bus DSI: %d lanes @ %d Mbps", DSI_LANES, DSI_MBPS);

    // 3) IO DBI: canal para enviar los comandos DCS al panel.
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_dbi_io_config_t dbi = { .virtual_channel = 0, .lcd_cmd_bits = 8, .lcd_param_bits = 8 };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi, &dbi, &io));

    // 4) DPI: flujo de pixeles (video).
    esp_lcd_dpi_panel_config_t dpi = {
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = DPI_CLK_MHZ,
        .virtual_channel    = 0,
        .in_color_format    = LCD_COLOR_FMT_RGB565,   // IDF v6: antes .pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565
        .num_fbs            = 1,
        .video_timing = {
            .h_size            = LCD_H_RES,
            .v_size            = LCD_V_RES,
            .hsync_back_porch  = 42,
            .hsync_pulse_width = 12,
            .hsync_front_porch = 42,
            .vsync_back_porch  = 2,
            .vsync_pulse_width = 8,
            .vsync_front_porch = 60,
        },
        // OJO IDF v6: se elimino .flags.use_dma2d. Para el patron de barras no hace falta DMA2D;
        // en el port a gui_display_dsi.c (blitting real) se usara esp_lcd_dpi_panel_enable_dma2d(panel, true).
    };

    // 5) Panel ST7701 con su tabla de init del vendor.
    st7701_vendor_config_t vendor = {
        .init_cmds      = vendor_init_cmds,
        .init_cmds_size = sizeof(vendor_init_cmds) / sizeof(vendor_init_cmds[0]),
        .flags          = { .use_mipi_interface = 1 },
        .mipi_config    = { .dsi_bus = dsi, .dpi_config = &dpi },
    };
    esp_lcd_panel_dev_config_t dev = {
        .bits_per_pixel = 16,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .reset_gpio_num = PIN_RST,
        .vendor_config  = &vendor,
    };
    esp_lcd_panel_handle_t panel = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io, &dev, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    ESP_LOGI(TAG, "Panel ST7701 %dx%d inicializado", LCD_H_RES, LCD_V_RES);
    return panel;
}

void app_main(void)
{
    ESP_LOGI(TAG, "==== ST7701 probe :: Waveshare P4 4.3\" (480x800) ====");

    backlight_init_on();
    esp_lcd_panel_handle_t panel = panel_init();

    // Barras de color verticales con el generador de patron del propio DPI (no necesita framebuffer).
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_set_pattern(panel, MIPI_DSI_PATTERN_BAR_VERTICAL));
    ESP_LOGI(TAG, ">>> Si el panel va, deberias ver BARRAS DE COLOR verticales en la pantalla <<<");

    int n = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "vivo (%d) — barras a la vista = ST7701 OK", n++);
    }
}
