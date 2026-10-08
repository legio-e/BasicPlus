/*
 * gui_display_dsi.c — backend de display del ESP32-P4 (MIPI-DSI, panel EK79007).
 *
 * Receta del panel = la del ejemplo oficial esp-idf/examples/peripherals/lcd/
 * mipi_dsi (ya validada en placa) + backlight por LEDC en GPIO26 (como el BSP;
 * el aviso "GPIO 26 not usable" es BENIGNO — el LEDC ata el pin igual; lo
 * importante es poner el duty, que es lo que faltaba).
 *
 * G3: solo panel + backlight + relleno ROJO (sin LVGL), para validar el display
 * en NUESTRO firmware.
 *
 * V7/H6 G5.a (26-sep) — con BPVM_GUI_PACK, LVGL viene del PACK y este fichero PUBLICA
 * la pantalla como 'PANT' desde la imagen (al final del fichero): el panel, la luz y
 * el táctil.
 *
 * #473/#477 (3-oct): fuera la mitad de BPVM_LVGL (LVGL dentro de la imagen: su display,
 * su táctil y su bombeo), muerta desde G5: ninguna imagen lo define.
 */
#include "gui_display_dsi.h"

#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ek79007.h"
#include "esp_lcd_st7701.h"      /* imagen única: 2º panel del catálogo (Waveshare 4.3") */
#include "esp_ldo_regulator.h"
#include "driver/ledc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"          /* G4: tick de LVGL (ms) */
#if defined(BPVM_GUI_PACK)
#include "bpvm_pant.h"          /* V7/H6 G5.a: el contrato del driver de pantalla ('PANT') */
#include "bpvm_bios.h"          /* V7/H6 G5.a: bpvm_bios_publica, el mismo registro que usan los packs */
#include "bpvm_machine.h"       /* V7 #669: bpvm_caps_reservar_sistema, los pines de la pantalla */
#endif
#include "freertos/FreeRTOS.h"  /* G4: vTaskDelay en el bombeo */
#include "freertos/task.h"
#include "driver/i2c_master.h"   /* G5: bus I2C del táctil (driver nuevo) */
#include "log.h"                 /* #424: el ritmo del lazo, al log (gated por log=1) */
#include "esp_lcd_panel_io.h"    /* G5: esp_lcd_new_panel_io_i2c */
#include "esp_lcd_touch.h"       /* G5: esp_lcd_touch_* (read_data / get_coordinates) */
#include "esp_lcd_touch_gt911.h" /* G5: driver GT911 */
#include "bpvm_gui.h"            /* G6: contrato de la costura bpvm_gui_disp_* */
#include "board_mgr_esp32.h"      /* imagen única: la placa se describe en el ENV */
#include <string.h>

static const char *TAG = "p4_gfx";

/* ===================== Catálogo de paneles (imagen única P4) =====================
 * Una imagen sirve a varias placas P4: lo que cambia por panel vive en una entrada
 * del catálogo (resolución, timings, velocidad DSI/DPI, polaridad del backlight,
 * transformación del táctil y la función de creación del IC). Los pines de control
 * (reset 27, backlight 26, I2C táctil 7/8) y el LDO del DPHY son IGUALES en las
 * placas conocidas y quedan comunes. Selección: el ENV (`display=st7701`) en
 * p4_panel_select(); sin clave → PANELS[0] (ek79007, la EV).
 * Params pin-a-pin data-driven = V4. */
typedef struct {
    const char *name;                 /* valor de la clave `display` del env */
    int hres, vres;                   /* resolución física del panel */
    int dsi_lanes, dsi_mbps;
    int dpi_clk_mhz;
    int hsync_pw, hsync_bp, hsync_fp;
    int vsync_pw, vsync_bp, vsync_fp;
    int bl_invert;                    /* 1 = backlight de lógica INVERTIDA (Waveshare) */
    int tp_swap_xy, tp_mirror_x, tp_mirror_y;   /* GT911 → coordenadas del panel */
    /* Crea el panel del vendor sobre el DBI (dev cfg común: reset GPIO27, RGB565)
     * y lo deja reseteado+inicializado. Cada IC aporta su vendor_config. */
    esp_err_t (*panel_create)(esp_lcd_panel_io_handle_t dbi_io,
                              esp_lcd_dsi_bus_handle_t dsi_bus,
                              esp_lcd_dpi_panel_config_t *dpi_cfg,
                              esp_lcd_panel_handle_t *out_panel);
} p4_panel_cfg_t;

static esp_err_t panel_create_ek79007(esp_lcd_panel_io_handle_t dbi_io,
                                      esp_lcd_dsi_bus_handle_t dsi_bus,
                                      esp_lcd_dpi_panel_config_t *dpi_cfg,
                                      esp_lcd_panel_handle_t *out_panel);
static esp_err_t panel_create_st7701(esp_lcd_panel_io_handle_t dbi_io,
                                     esp_lcd_dsi_bus_handle_t dsi_bus,
                                     esp_lcd_dpi_panel_config_t *dpi_cfg,
                                     esp_lcd_panel_handle_t *out_panel);

static const p4_panel_cfg_t PANELS[] = {
    /* EK79007 1024x600 (P4-Function-EV). Tiempos = ejemplo oficial mipi_dsi
     * (refresh 60 Hz); DPI 52 = macro EK79007_..._60HZ_CONFIG del driver (NO 48);
     * porches h = 160 (los del driver). Táctil mirror 1/1 (como el BSP del kit). */
    { .name = "ek79007", .hres = 1024, .vres = 600,
      .dsi_lanes = 2, .dsi_mbps = 1000, .dpi_clk_mhz = 52,
      .hsync_pw = 10, .hsync_bp = 160, .hsync_fp = 160,
      .vsync_pw = 1,  .vsync_bp = 23,  .vsync_fp = 12,
      .bl_invert = 0,
      .tp_swap_xy = 0, .tp_mirror_x = 1, .tp_mirror_y = 1,
      .panel_create = panel_create_ek79007 },
    /* ST7701 480x800 PORTRAIT (Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3). Valores del
     * BSP de Waveshare, validados en placa por la sonda st7701_probe y el firmware
     * esp32p4-ws. OJO: backlight de lógica INVERTIDA; táctil SIN transformar. */
    { .name = "st7701", .hres = 480, .vres = 800,
      .dsi_lanes = 2, .dsi_mbps = 500, .dpi_clk_mhz = 30,
      .hsync_pw = 12, .hsync_bp = 42, .hsync_fp = 42,
      .vsync_pw = 8,  .vsync_bp = 2,  .vsync_fp = 60,
      .bl_invert = 1,
      .tp_swap_xy = 0, .tp_mirror_x = 0, .tp_mirror_y = 0,
      .panel_create = panel_create_st7701 },
};

/* Panel VIGENTE. Default = 1ª entrada (ek79007); p4_panel_select() lo cambia si
 * el env lo pide (se llama UNA vez, antes del primer uso de s_cfg). */
static const p4_panel_cfg_t *s_cfg = &PANELS[0];

/* Clave del env que elige el panel. CANÓNICA (minúsculas), igual que "psram" en
 * el RP2350: el firmware compara EXACTO. */
#define ENV_KEY_DISPLAY  "display"

/* #311 — selección runtime del panel por el ENV (`display=st7701`). Antes esto
 * salía de /sys/board.json y era la última config de HARDWARE que vivía dentro
 * del FS: se perdía al formatear, viajaba en el sistema de ficheros de la app y
 * dependía de una capa (FS, estado 3) que arranca DESPUÉS de conocerse la placa.
 * El env está en su propia partición, sobrevive al reflasheo y ya está leído en
 * el estado 2 — mismo sitio y mismo trato que `psram`, que es de lo que se
 * trataba: una sola manera de decir cómo es esta placa.
 * Sin clave o valor fuera de catálogo → queda el default (la EV no configura
 * nada). El backlight viene con el panel: `bl_invert` es un campo de la entrada,
 * así que elegir el panel elige también su polaridad. */
static void p4_panel_select(void)
{
    char name[24] = {0};
    if (bpvm_env_get(board_mgr_env(), ENV_KEY_DISPLAY, name, sizeof name) < 0 || name[0] == '\0') {
        ESP_LOGI(TAG, "panel: env sin \"%s\" — default %s", ENV_KEY_DISPLAY, s_cfg->name);
        return;
    }
    for (size_t i = 0; i < sizeof PANELS / sizeof PANELS[0]; i++) {
        if (strcmp(PANELS[i].name, name) == 0) {
            s_cfg = &PANELS[i];
            ESP_LOGI(TAG, "panel: env %s=\"%s\" -> %dx%d (bl_invert=%d)",
                     ENV_KEY_DISPLAY, s_cfg->name, s_cfg->hres, s_cfg->vres, s_cfg->bl_invert);
            return;
        }
    }
    ESP_LOGW(TAG, "panel: %s=\"%s\" NO esta en el catalogo (ek79007, st7701) — default %s",
             ENV_KEY_DISPLAY, name, s_cfg->name);
}

#define LDO_CHAN     3       /* LDO_VO3 -> VDD_MIPI_DPHY (igual en las placas conocidas) */
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
        .flags.output_invert = s_cfg->bl_invert,   /* Waveshare: 1 (duty 1023 = ON) */
    };
    esp_err_t ec = ledc_channel_config(&ccfg);
    esp_err_t ed = ledc_set_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CH, 1023);   /* 100 % */
    esp_err_t eu = ledc_update_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CH);
    ESP_LOGI(TAG, "backlight GPIO%d LEDC: timer=%s chan=%s setduty=%s upd=%s",
             BL_GPIO, esp_err_to_name(et), esp_err_to_name(ec),
             esp_err_to_name(ed), esp_err_to_name(eu));
}

/* Creación del EK79007 (P4-Function-EV): vendor cfg mínima + new_panel + reset/init.
 * El EK79007 no soporta disp_on_off (el BSP no lo llama); el DPI escanea tras init. */
static esp_err_t panel_create_ek79007(esp_lcd_panel_io_handle_t dbi_io,
                                      esp_lcd_dsi_bus_handle_t dsi_bus,
                                      esp_lcd_dpi_panel_config_t *dpi_cfg,
                                      esp_lcd_panel_handle_t *out_panel)
{
    ek79007_vendor_config_t vendor = {
        .mipi_config = { .dsi_bus = dsi_bus, .dpi_config = dpi_cfg },
    };
    esp_lcd_panel_dev_config_t dev_cfg = {
        .reset_gpio_num = LCD_RST_GPIO,   /* GPIO27 — reset HW del panel (como el BSP) */
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config  = &vendor,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ek79007(dbi_io, &dev_cfg, out_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*out_panel));   /* pulso de reset por GPIO27 */
    ESP_ERROR_CHECK(esp_lcd_panel_init(*out_panel));
    return ESP_OK;
}

/* Tabla de init DCS del ST7701 — COPIADA LITERAL del BSP de Waveshare (misma que la sonda
 * st7701_probe y el firmware esp32p4-ws). El EK79007 la lleva en su driver; el ST7701
 * la recibe en vendor_config. */
static const st7701_lcd_init_cmd_t s_st7701_init_cmds[] = {
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
    {0x11, (uint8_t[]){0x00}, 0, 120},   /* Sleep Out + 120 ms */
    {0x29, (uint8_t[]){0x00}, 0, 0},     /* Display On */
};

/* Creación del ST7701 (Waveshare 4.3"): vendor cfg con la tabla DCS + new_panel +
 * reset/init + disp_on_off — el ST7701 SÍ lo usa (cmd 0x29), a diferencia del EK79007. */
static esp_err_t panel_create_st7701(esp_lcd_panel_io_handle_t dbi_io,
                                     esp_lcd_dsi_bus_handle_t dsi_bus,
                                     esp_lcd_dpi_panel_config_t *dpi_cfg,
                                     esp_lcd_panel_handle_t *out_panel)
{
    st7701_vendor_config_t vendor = {
        .init_cmds      = s_st7701_init_cmds,
        .init_cmds_size = sizeof(s_st7701_init_cmds) / sizeof(s_st7701_init_cmds[0]),
        .flags          = { .use_mipi_interface = 1 },
        .mipi_config    = { .dsi_bus = dsi_bus, .dpi_config = dpi_cfg },
    };
    esp_lcd_panel_dev_config_t dev_cfg = {
        .reset_gpio_num = LCD_RST_GPIO,   /* GPIO27 — reset HW del panel (como el BSP) */
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config  = &vendor,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(dbi_io, &dev_cfg, out_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*out_panel));   /* pulso de reset por GPIO27 */
    ESP_ERROR_CHECK(esp_lcd_panel_init(*out_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*out_panel, true));
    return ESP_OK;
}

/* Inicializa DPHY (LDO) + bus DSI + el panel del CATÁLOGO (RGB565) + reset/init.
 * Devuelve el handle del panel DPI (idempotente). */
static esp_lcd_panel_handle_t p4_dsi_panel_init(void)
{
    if (s_panel) return s_panel;

    /* Imagen única: elegir panel del catálogo ANTES de consumir s_cfg (todos los
     * caminos —costura LVGL y tests de diagnóstico— pasan por aquí primero). */
    p4_panel_select();

    /* DPHY: 2.5 V por el LDO ch3 (ANTES del bus DSI). */
    esp_ldo_channel_handle_t ldo = NULL;
    esp_ldo_channel_config_t ldo_cfg = { .chan_id = LDO_CHAN, .voltage_mv = LDO_MV };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &ldo));

    /* Bus DSI — inicializa la DPHY. Lanes/velocidad del catálogo. */
    esp_lcd_dsi_bus_handle_t dsi_bus = NULL;
    esp_lcd_dsi_bus_config_t bus_cfg = {
        .bus_id             = 0,
        .num_data_lanes     = s_cfg->dsi_lanes,
        .lane_bit_rate_mbps = s_cfg->dsi_mbps,
    };
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus));

    /* Canal de comandos DBI (init del panel). */
    esp_lcd_panel_io_handle_t dbi_io = NULL;
    esp_lcd_dbi_io_config_t dbi_cfg = { .virtual_channel = 0, .lcd_cmd_bits = 8, .lcd_param_bits = 8 };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_cfg, &dbi_io));

    /* Panel DPI, RGB565 (FB en PSRAM). Resolución/timings del catálogo. */
    esp_lcd_dpi_panel_config_t dpi_cfg = {
        .virtual_channel    = 0,
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = s_cfg->dpi_clk_mhz,
        .in_color_format    = LCD_COLOR_FMT_RGB565,
        .num_fbs            = 1,    /* FB interno: el draw_bitmap escribe aquí y el DPI lo escanea */
        .video_timing = {
            .h_size = s_cfg->hres, .v_size = s_cfg->vres,
            .hsync_pulse_width = s_cfg->hsync_pw, .hsync_back_porch = s_cfg->hsync_bp, .hsync_front_porch = s_cfg->hsync_fp,
            .vsync_pulse_width = s_cfg->vsync_pw, .vsync_back_porch = s_cfg->vsync_bp, .vsync_front_porch = s_cfg->vsync_fp,
        },
    };
    ESP_ERROR_CHECK(s_cfg->panel_create(dbi_io, dsi_bus, &dpi_cfg, &s_panel));

    ESP_LOGI(TAG, "panel %s %dx%d RGB565 (reset GPIO%d) inicializado",
             s_cfg->name, s_cfg->hres, s_cfg->vres, LCD_RST_GPIO);
    return s_panel;
}

void p4_gfx_smoke_test(void)
{
    p4_dsi_panel_init();
    p4_backlight_on();

    size_t px = (size_t) s_cfg->hres * s_cfg->vres;
    uint16_t *fb = (uint16_t *) heap_caps_malloc(px * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (fb == NULL) { ESP_LOGE(TAG, "sin PSRAM para el framebuffer"); return; }
    for (size_t i = 0; i < px; i++) fb[i] = 0xF800;   /* rojo en RGB565 */

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_panel, 0, 0, s_cfg->hres, s_cfg->vres, fb));
    ESP_LOGI(TAG, "G3: ROJO volcado. Pantalla roja => DSI+%s+backlight+PSRAM OK en nuestro firmware", s_cfg->name);
    /* fb se queda vivo a propósito (el draw del DPI puede ser asíncrono). */
}

/* -------------------- G5: táctil GT911 @0x14 (I2C SDA=GPIO7/SCL=GPIO8) -------------------- */
#define TOUCH_I2C_SDA   7
#define TOUCH_I2C_SCL   8
#define TOUCH_I2C_PORT  0

static esp_lcd_touch_handle_t s_tp = NULL;

/* Intenta abrir el GT911 en una dirección concreta. Devuelve el handle o NULL (sin
 * abortar). flags mirror_x/y = 1 como el BSP, para que el toque cuadre con el panel. */
static esp_lcd_touch_handle_t p4_touch_try(i2c_master_bus_handle_t bus, uint16_t addr)
{
    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_cfg.dev_addr = addr;
    if (esp_lcd_new_panel_io_i2c(bus, &tp_io_cfg, &tp_io) != ESP_OK) return NULL;

    esp_lcd_touch_config_t tp_cfg = {
        .x_max        = s_cfg->hres,
        .y_max        = s_cfg->vres,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .levels = { .reset = 0, .interrupt = 0 },
        /* Transformación GT911→panel del catálogo (EV: mirror 1/1 como su BSP;
         * Waveshare: todo 0 — su BSP no transforma). */
        .flags  = { .swap_xy  = s_cfg->tp_swap_xy,
                    .mirror_x = s_cfg->tp_mirror_x,
                    .mirror_y = s_cfg->tp_mirror_y },
    };
    esp_lcd_touch_handle_t tp = NULL;
    if (esp_lcd_touch_new_i2c_gt911(tp_io, &tp_cfg, &tp) != ESP_OK) {
        esp_lcd_panel_io_del(tp_io);   /* limpia el io fallido antes de reintentar */
        return NULL;
    }
    ESP_LOGI(TAG, "GT911 init OK en 0x%02X (I2C SDA=%d SCL=%d)", addr, TOUCH_I2C_SDA, TOUCH_I2C_SCL);
    return tp;
}

/* Bus I2C nuevo + GT911. La dirección la fija el pin INT (NC en esta placa) durante
 * el reset por GPIO27 -> sale 0x14 o 0x5D de forma no determinista. Probamos AMBAS.
 * Si ninguna responde, NO abortamos: seguimos sin táctil (el botón se ve, no reacciona). */
static esp_lcd_touch_handle_t p4_touch_init(void)
{
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_cfg = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .sda_io_num                   = TOUCH_I2C_SDA,
        .scl_io_num                   = TOUCH_I2C_SCL,
        .i2c_port                     = TOUCH_I2C_PORT,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t eb = i2c_new_master_bus(&bus_cfg, &i2c_bus);
    if (eb != ESP_OK) {
        /* V7 #669: sin abortar (antes, ESP_ERROR_CHECK: la placa se reiniciaba). Sus pines son de la pantalla, pero un
         * programa pudo abrir antes el I2C 0 con OTROS: el puerto está tomado. La pantalla sigue, sin táctil, y se dice. */
        ESP_LOGE(TAG, "el I2C %d del tactil no se pudo abrir (%s): sin tactil", TOUCH_I2C_PORT, esp_err_to_name(eb));
        log_printf("pantalla: el I2C %d del tactil no se pudo abrir (%s): la GUI se vera pero no reaccionara (#669)",
                   TOUCH_I2C_PORT, esp_err_to_name(eb));
        return NULL;
    }

    /* El GT911 comparte el reset con el panel (GPIO27) y tarda ~100-200 ms en
     * responder por I2C tras ese reset. Lo sondeábamos ~10 ms después -> "i2c
     * transaction failed". (El BSP no fallaba porque su arranque metía de sobra esa
     * demora antes de tocar el táctil.) HW idéntico al de ayer -> era esto, no la
     * dirección. */
    vTaskDelay(pdMS_TO_TICKS(200));

    esp_lcd_touch_handle_t tp = p4_touch_try(i2c_bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP); /* 0x14 */
    if (!tp) {
        ESP_LOGW(TAG, "GT911 no respondió en 0x14; probando 0x5D...");
        tp = p4_touch_try(i2c_bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS);                            /* 0x5D */
    }
    if (!tp) ESP_LOGE(TAG, "GT911 no responde en 0x14 ni 0x5D (revisar FPC/cableado del táctil)");
    return tp;
}

#if defined(BPVM_GUI_PACK)
/* ═════════ V7/H6 G5.a (26-sep) — LA PANTALLA DEL P4, PUBLICADA COMO 'PANT' DESDE LA IMAGEN ═════════
 * LVGL ya no va en la imagen: viene del pack (lvgl.npk.RISCV) y busca su driver de pantalla por la marca 'PANT',
 * como en la C5 y la C6. El panel del P4 —MIPI-DSI y DPI— no se puede prestar por la BIOS (es un periférico de la
 * familia), así que su driver se queda aquí y publica lo mismo que DriverSpi (bpvm_pant.h): init, size, flush,
 * rotate y touch. El formato no se publica: RGB565 tal cual, lo que escanea el DPI.
 *
 * EL GIRO. Un panel DPI no gira por hardware: el LVGL de la imagen giraba cada franja por software en su flush
 * (lv_draw_sw_rotate + lv_display_rotate_area) y el punto del táctil en su indev. El LVGL del pack pinta con el ancho
 * y el alto cambiados y no sabe de giros, así que las tres cosas se hacen aquí, con las MISMAS cuentas que LVGL 9.2.2
 * (lv_display.c, lv_draw_sw.c, lv_indev.c): Gui.setRotation deja la pantalla exactamente como antes. W×H es el panel
 * a 0° (la resolución física del catálogo). */
static int       s_pant_rot = 0;            /* 0, 90, 180 o 270 */
static int       s_pant_listo = 0;          /* el panel, la luz y el táctil, UNA vez (el bus del táctil no se abre dos) */
static uint16_t *s_pant_giro = NULL;        /* la franja girada (PSRAM, sólo crece) */
static size_t    s_pant_giro_px = 0;

static int pant_init(void)
{
    if (!s_pant_listo) {
        p4_dsi_panel_init();
        p4_backlight_on();
        s_tp = p4_touch_init();
        if (s_tp == NULL) ESP_LOGW(TAG, "sin tactil: la GUI se vera pero no reaccionara");
        s_pant_listo = 1;
    }
    return s_panel ? 0 : -1;
}

static int pant_size(int *w, int *h)
{
    p4_panel_select();           /* idempotente: sólo lee el ENV (`display=`) */
    *w = s_cfg->hres;
    *h = s_cfg->vres;
    return 1;
}

static int pant_rotate(int deg)
{
    if (deg != 0 && deg != 90 && deg != 180 && deg != 270) return -1;
    s_pant_rot = deg;
    return 0;                    /* girado: el pack de LVGL cambia su resolución y repinta */
}

static void pant_flush(int x1, int y1, int x2, int y2, const uint8_t *px)
{
    if (s_panel == NULL) return;
    if (s_pant_rot == 0) {
        esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2 + 1, y2 + 1, px);
        return;
    }
    const int32_t w = x2 - x1 + 1, h = y2 - y1 + 1;
    const size_t n = (size_t) w * (size_t) h;
    if (n > s_pant_giro_px) {
        uint16_t *nb = (uint16_t *) heap_caps_realloc(s_pant_giro, n * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
        if (nb == NULL) {
            ESP_LOGE(TAG, "sin PSRAM para girar la franja (%u px) — franja saltada", (unsigned) n);
            return;
        }
        s_pant_giro = nb;
        s_pant_giro_px = n;
    }
    const uint16_t *src = (const uint16_t *) px;
    uint16_t *dst = s_pant_giro;
    /* Los píxeles: rotate90/180/270_rgb565 de lv_draw_sw.c, con los pasos en píxeles (el origen, w; el destino, h a
     * 90 y 270, w a 180). */
    if (s_pant_rot == 90) {
        for (int32_t x = 0; x < w; ++x) {
            int32_t di = w - x - 1, si = x;
            for (int32_t y = 0; y < h; ++y) { dst[di * h + y] = src[si]; si += w; }
        }
    } else if (s_pant_rot == 180) {
        for (int32_t y = 0; y < h; ++y) {
            int32_t di = (h - y - 1) * w, si = y * w;
            for (int32_t x = 0; x < w; ++x) dst[di + w - x - 1] = src[si + x];
        }
    } else {
        for (int32_t x = 0; x < w; ++x) {
            int32_t di = x * h, si = x;
            for (int32_t y = 0; y < h; ++y) { dst[di + (h - y - 1)] = src[si]; si += w; }
        }
    }
    /* La zona: lv_display_rotate_area, con W×H = el panel a 0°. (a1,b1)-(a2,b2) es la zona FÍSICA. */
    const int32_t W = s_cfg->hres, H = s_cfg->vres;
    int32_t a1, b1, a2, b2;
    if (s_pant_rot == 90) {
        b2 = H - x1 - 1; a1 = y1; a2 = a1 + h - 1; b1 = b2 - w + 1;
    } else if (s_pant_rot == 180) {
        b2 = H - y1 - 1; b1 = b2 - h + 1; a2 = W - x1 - 1; a1 = a2 - w + 1;
    } else {
        a1 = W - y2 - 1; b2 = x2; a2 = a1 + h - 1; b1 = b2 - w + 1;
    }
    esp_lcd_panel_draw_bitmap(s_panel, a1, b1, a2 + 1, b2 + 1, dst);
}

static int pant_touch(int *x, int *y)
{
    if (s_tp == NULL) return 0;
    uint16_t tx = 0, ty = 0;
    uint8_t  cnt = 0;
    esp_lcd_touch_read_data(s_tp);
    bool pulsado = esp_lcd_touch_get_coordinates(s_tp, &tx, &ty, NULL, &cnt, 1);
    if (!pulsado || cnt == 0) return 0;
    /* El punto: indev_pointer_proc de lv_indev.c —del panel a 0° a la orientación actual—. */
    int32_t px = tx, py = ty;
    const int32_t W = s_cfg->hres, H = s_cfg->vres;
    if (s_pant_rot == 180 || s_pant_rot == 270) { px = W - px - 1; py = H - py - 1; }
    if (s_pant_rot == 90  || s_pant_rot == 270) { int32_t t = py; py = px; px = H - t - 1; }
    *x = (int) px;
    *y = (int) py;
    return 1;
}

static const bpvm_pack_sym_t s_pant_syms[] = {
    { "init",   (bpvm_pack_fn_t) pant_init   },
    { "size",   (bpvm_pack_fn_t) pant_size   },
    { "flush",  (bpvm_pack_fn_t) pant_flush  },
    { "rotate", (bpvm_pack_fn_t) pant_rotate },
    { "touch",  (bpvm_pack_fn_t) pant_touch  },
};
static const bpvm_pack_api_t s_pant_api = {
    BPVM_PANT_MARCA, BPVM_PANT_API_VER,
    (uint32_t) (sizeof s_pant_syms / sizeof s_pant_syms[0]), s_pant_syms
};

/* V7 #669 (8-oct) — LOS PINES DE LA PANTALLA, del sistema desde que se publica su driver (Eduardo: «Los pines de la
 * pantalla deberían configurarse al mismo tiempo que el driver de la pantalla y en principio no utilizarse para otras
 * cosas»). El panel se configura en su primer uso (pant_init), pero sus pines son suyos desde el arranque: un programa
 * que los pida antes no puede dejarla sin luz ni sin reset —y desde #667, al soltarlos, volvían al reset—. Fuera de
 * Machine.pins(), como los de la flash (#574). El LEDC de la luz (canal 1, timer 1) ya lo esquivaba el PWM. */
static void p4_pant_reservar_pines(void)
{
    static const struct { int pin; const char* que; } P[] = {
        { BL_GPIO,       "la retroiluminacion" },
        { LCD_RST_GPIO,  "el reset del panel y del tactil" },
        { TOUCH_I2C_SDA, "el SDA del tactil" },
        { TOUCH_I2C_SCL, "el SCL del tactil" },
    };
    char err[96];
    int n = 0;
    for (size_t i = 0; i < sizeof P / sizeof P[0]; i++) {
        if (bpvm_caps_reservar_sistema(P[i].pin, BPVM_PIN_SIS_PANTALLA, err, sizeof err) == 0) n++;
        else log_printf("pantalla: el GPIO%d (%s) NO se pudo reservar: %s", P[i].pin, P[i].que, err);
    }
    log_printf("pantalla: %d pines suyos (la luz, el reset y el tactil): fuera de Machine.pins() (#669)", n);
}

void p4_pant_publicar(void)
{
    /* Al arrancar, antes que ningún pack: el de LVGL la busca en su PRIMER USO (el primer Gui.Screen), así que el
     * orden da igual; y si algún pack de driver quisiera publicar otra 'PANT', el registro se lo niega con nombre
     * (-3) en vez de pisar la de la imagen. */
    int r = bpvm_bios_publica(BPVM_PANT_MARCA, &s_pant_api);
    if (r != 0) {
        ESP_LOGE(TAG, "publica('PANT') = %d: la GUI del pack no tendra pantalla", r);
        log_printf("pantalla: el P4 NO pudo publicar 'PANT' (publica = %d)", r);
    } else {
        log_printf("pantalla: el P4 publica 'PANT' desde la imagen (MIPI-DSI, el panel del ENV, tactil GT911)");
        p4_pant_reservar_pines();   /* V7 #669: al mismo tiempo que su driver */
    }
}
#endif /* BPVM_GUI_PACK */
