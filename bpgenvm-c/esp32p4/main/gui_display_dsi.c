/*
 * gui_display_dsi.c — backend de display del ESP32-P4 (MIPI-DSI, panel EK79007).
 *
 * Receta del panel = la del ejemplo oficial esp-idf/examples/peripherals/lcd/
 * mipi_dsi (ya validada en placa) + backlight por LEDC en GPIO26 (como el BSP;
 * el aviso "GPIO 26 not usable" es BENIGNO — el LEDC ata el pin igual; lo
 * importante es poner el duty, que es lo que faltaba).
 *
 * G3 (este paso): solo panel + backlight + relleno ROJO (sin LVGL), para validar
 * el display en NUESTRO firmware. La costura LVGL bpvm_gui_disp_* (bajo BPVM_LVGL)
 * llega en G4 reutilizando p4_dsi_panel_init().
 */
#include "gui_display_dsi.h"

#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ek79007.h"
#include "esp_ldo_regulator.h"
#include "driver/ledc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "p4_gfx";

/* Panel EK79007 1024x600, MIPI-DSI 2-lane @1000 Mbps. Tiempos = ejemplo oficial
 * (refresh 60 Hz: 48 MHz / (10+120+120+1024) / (1+20+10+600)). */
#define LCD_H_RES    1024
#define LCD_V_RES    600
#define DPI_CLK_MHZ  52      /* macro EK79007_..._60HZ_CONFIG del driver (NO 48) */
#define LCD_HSYNC    10
#define LCD_HBP      160     /* porches del driver = 160 (yo tenía 120, del ejemplo malo) */
#define LCD_HFP      160
#define LCD_VSYNC    1
#define LCD_VBP      23
#define LCD_VFP      12
#define DSI_LANES    2
#define DSI_MBPS     1000
#define LDO_CHAN     3       /* LDO_VO3 -> VDD_MIPI_DPHY */
#define LDO_MV       2500

/* Reset HW del panel = GPIO27 (BSP_LCD_RST). CLAVE: el EK79007 NO arranca fiable
 * sin él. El ejemplo usaba -1 -> pantalla negra; el BSP usa 27 -> rojo. */
#define LCD_RST_GPIO 27

/* Backlight: PWM por LEDC en GPIO26 (pin del kit, DuPont J6 PWM -> GPIO26). */
#define BL_GPIO      26
#define BL_LEDC_CH   LEDC_CHANNEL_1   /* mismos que el BSP (timer 1 / canal 1) */
#define BL_LEDC_TMR  LEDC_TIMER_1

static esp_lcd_panel_handle_t s_panel = NULL;

/* LEDC backlight a 100 %. Réplica EXACTA del bsp_display_brightness_init (que sí
 * encendió en la prueba del rojo): timer 1, canal 1, 10-bit, 5 kHz. Logueamos
 * CADA retorno — en el G3 anterior las llamadas fallaron en silencio (no salía ni
 * el aviso "GPIO 26 not usable") y la pantalla quedó oscura. */
static void p4_backlight_on(void)
{
    ledc_timer_config_t tcfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num       = BL_LEDC_TMR,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t et = ledc_timer_config(&tcfg);
    ledc_channel_config_t ccfg = {
        .gpio_num   = BL_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = BL_LEDC_CH,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = BL_LEDC_TMR,
        .duty       = 0,
        .hpoint     = 0,
    };
    esp_err_t ec = ledc_channel_config(&ccfg);
    esp_err_t ed = ledc_set_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CH, 1023);   /* 100 % */
    esp_err_t eu = ledc_update_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CH);
    ESP_LOGI(TAG, "backlight GPIO%d LEDC: timer=%s chan=%s setduty=%s upd=%s",
             BL_GPIO, esp_err_to_name(et), esp_err_to_name(ec),
             esp_err_to_name(ed), esp_err_to_name(eu));
}

/* Inicializa DPHY (LDO) + bus DSI + panel EK79007 (RGB565) + reset/init + display
 * ON. Devuelve el handle del panel DPI (idempotente). */
static esp_lcd_panel_handle_t p4_dsi_panel_init(void)
{
    if (s_panel) return s_panel;

    /* DPHY: 2.5 V por el LDO ch3 (ANTES del bus DSI). */
    esp_ldo_channel_handle_t ldo = NULL;
    esp_ldo_channel_config_t ldo_cfg = { .chan_id = LDO_CHAN, .voltage_mv = LDO_MV };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &ldo));

    /* Bus DSI (2-lane, 1 Gbps) — inicializa la DPHY. */
    esp_lcd_dsi_bus_handle_t dsi_bus = NULL;
    esp_lcd_dsi_bus_config_t bus_cfg = {
        .bus_id             = 0,
        .num_data_lanes     = DSI_LANES,
        .lane_bit_rate_mbps = DSI_MBPS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus));

    /* Canal de comandos DBI (init del panel). */
    esp_lcd_panel_io_handle_t dbi_io = NULL;
    esp_lcd_dbi_io_config_t dbi_cfg = { .virtual_channel = 0, .lcd_cmd_bits = 8, .lcd_param_bits = 8 };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_cfg, &dbi_io));

    /* Panel DPI EK79007, RGB565 (FB ~1.2 MB en PSRAM). */
    esp_lcd_dpi_panel_config_t dpi_cfg = {
        .virtual_channel    = 0,
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = DPI_CLK_MHZ,
        .in_color_format    = LCD_COLOR_FMT_RGB565,
        .num_fbs            = 1,    /* FB interno: el draw_bitmap escribe aquí y el DPI lo escanea */
        .video_timing = {
            .h_size = LCD_H_RES, .v_size = LCD_V_RES,
            .hsync_pulse_width = LCD_HSYNC, .hsync_back_porch = LCD_HBP, .hsync_front_porch = LCD_HFP,
            .vsync_pulse_width = LCD_VSYNC, .vsync_back_porch = LCD_VBP, .vsync_front_porch = LCD_VFP,
        },
    };
    ek79007_vendor_config_t vendor = {
        .mipi_config = { .dsi_bus = dsi_bus, .dpi_config = &dpi_cfg },
    };
    esp_lcd_panel_dev_config_t dev_cfg = {
        .reset_gpio_num = LCD_RST_GPIO,   /* GPIO27 — reset HW del panel (como el BSP) */
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config  = &vendor,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ek79007(dbi_io, &dev_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));   /* pulso de reset por GPIO27 */
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    /* El EK79007 no soporta disp_on_off (el BSP no lo llama); el DPI escanea tras init. */

    ESP_LOGI(TAG, "panel EK79007 1024x600 RGB565 (reset GPIO%d) inicializado", LCD_RST_GPIO);
    return s_panel;
}

void p4_gfx_smoke_test(void)
{
    p4_dsi_panel_init();
    p4_backlight_on();

    size_t px = (size_t) LCD_H_RES * LCD_V_RES;
    uint16_t *fb = (uint16_t *) heap_caps_malloc(px * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (fb == NULL) { ESP_LOGE(TAG, "sin PSRAM para el framebuffer"); return; }
    for (size_t i = 0; i < px; i++) fb[i] = 0xF800;   /* rojo en RGB565 */

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_panel, 0, 0, LCD_H_RES, LCD_V_RES, fb));
    ESP_LOGI(TAG, "G3: ROJO volcado. Pantalla roja => DSI+EK79007+backlight+PSRAM OK en nuestro firmware");
    /* fb se queda vivo a propósito (el draw del DPI puede ser asíncrono). */
}
