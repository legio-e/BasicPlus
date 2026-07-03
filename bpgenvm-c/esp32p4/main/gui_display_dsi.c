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
#include "esp_timer.h"          /* G4: tick de LVGL (ms) */
#include "lvgl.h"               /* G4: LVGL 9.2.2 vendorizada (componente IDF) */
#include "freertos/FreeRTOS.h"  /* G4: vTaskDelay en el bombeo */
#include "freertos/task.h"
#include "driver/i2c_master.h"   /* G5: bus I2C del táctil (driver nuevo) */
#include "esp_lcd_panel_io.h"    /* G5: esp_lcd_new_panel_io_i2c */
#include "esp_lcd_touch.h"       /* G5: esp_lcd_touch_* (read_data / get_coordinates) */
#include "esp_lcd_touch_gt911.h" /* G5: driver GT911 */
#include "bpvm_gui.h"            /* G6: contrato de la costura bpvm_gui_disp_* */

static const char *TAG = "p4_gfx";

/* ===================== Catálogo de paneles (imagen única P4) =====================
 * Una imagen sirve a varias placas P4: lo que cambia por panel vive en una entrada
 * del catálogo (resolución, timings, velocidad DSI/DPI, polaridad del backlight,
 * transformación del táctil y la función de creación del IC). Los pines de control
 * (reset 27, backlight 26, I2C táctil 7/8) y el LDO del DPHY son IGUALES en las
 * placas conocidas y quedan comunes. Selección: hoy fija (1ª entrada); el paso
 * board.json la hará runtime. Params pin-a-pin data-driven = V4. */
typedef struct {
    const char *name;                 /* clave para /sys/board.json */
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
};

/* Panel VIGENTE (por ahora fijo a la 1ª entrada; el paso board.json lo hará runtime). */
static const p4_panel_cfg_t *s_cfg = &PANELS[0];

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

/* Inicializa DPHY (LDO) + bus DSI + el panel del CATÁLOGO (RGB565) + reset/init.
 * Devuelve el handle del panel DPI (idempotente). */
static esp_lcd_panel_handle_t p4_dsi_panel_init(void)
{
    if (s_panel) return s_panel;

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

/* ============================ G4 — LVGL 9.2.2 ============================
 * Prueba de LVGL en NUESTRO firmware: panel (reutilizado de G3) + lv_display con
 * flush vía esp_lcd_panel_draw_bitmap + un botón con label. Sin táctil todavía
 * (G5 añade el GT911 como lv_indev). La costura portable bpvm_gui_disp_*
 * (bpvm_gui.h, bajo BPVM_LVGL) se cablea en G6 junto a gui.c; aquí va una prueba
 * autónoma para validar el toolchain LVGL + el render.
 *
 * Color: LVGL con LV_COLOR_DEPTH 16 produce RGB565 nativo (lo activa BPVM_BOARD_P4
 * en include/lv_conf.h); el panel lo escanea igual que el 0xF800 del smoke test. */
static lv_display_t *s_lv_disp = NULL;

static uint32_t p4_lv_tick_ms(void)
{
    return (uint32_t) (esp_timer_get_time() / 1000);
}

/* Orientacion del display al arrancar: el EK79007 es LANDSCAPE nativo (1024x600) →
 * default 0 (sin coste de giro). Gui.setRotation() la cambia en runtime. */
#define LCD_ROTATION LV_DISPLAY_ROTATION_0

/* Orientacion VIGENTE: arranca en LCD_ROTATION y Gui.setRotation() la cambia en
 * caliente (bpvm_gui_disp_set_rotation). Si se llama antes de crear el display,
 * queda como orientacion de arranque (disp_init la aplica). */
static lv_display_rotation_t s_rotation = LCD_ROTATION;

/* Buffer de giro para el flush (PSRAM, grow-only; tope = tamano del draw buffer). */
static void  *s_rot_buf      = NULL;
static size_t s_rot_buf_size = 0;

/* Vuelca la zona renderizada (RGB565) al framebuffer del panel DPI. Sin DMA2D el
 * draw_bitmap copia por CPU de forma síncrona -> flush_ready justo después.
 * Con rotacion != 0 gira primero por software — patron OFICIAL de los drivers LVGL
 * (lv_linux_fbdev.c / lv_sdl_window.c) portado de esp32p4-ws: lv_draw_sw_rotate +
 * lv_display_rotate_area; el core NO gira pixeles en 9.2, es contrato del flush. */
static void p4_lv_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

    if (rotation != LV_DISPLAY_ROTATION_0) {
        int32_t w = lv_area_get_width(area);
        int32_t h = lv_area_get_height(area);
        lv_color_format_t cf = lv_display_get_color_format(disp);
        uint32_t px_size = lv_color_format_get_size(cf);

        size_t buf_size = (size_t) w * h * px_size;
        if (s_rot_buf == NULL || s_rot_buf_size < buf_size) {
            void *nb = heap_caps_realloc(s_rot_buf, buf_size, MALLOC_CAP_SPIRAM);
            if (nb == NULL) {
                ESP_LOGE(TAG, "sin PSRAM para el buffer de rotacion (%u B) — frame saltado",
                         (unsigned) buf_size);
                lv_display_flush_ready(disp);
                return;
            }
            s_rot_buf      = nb;
            s_rot_buf_size = buf_size;
        }

        uint32_t w_stride = lv_draw_buf_width_to_stride(w, cf);
        uint32_t h_stride = lv_draw_buf_width_to_stride(h, cf);
        /* 90/270 giran w<->h (stride destino = h_stride); 180 mantiene (w_stride). */
        lv_draw_sw_rotate(px_map, s_rot_buf, w, h, w_stride,
                          (rotation == LV_DISPLAY_ROTATION_180) ? w_stride : h_stride,
                          rotation, cf);
        px_map = (uint8_t *) s_rot_buf;

        lv_area_t rotated_area = *area;
        lv_display_rotate_area(disp, &rotated_area);   /* area logica -> area fisica del panel */
        esp_lcd_panel_draw_bitmap(s_panel, rotated_area.x1, rotated_area.y1,
                                  rotated_area.x2 + 1, rotated_area.y2 + 1, px_map);
        lv_display_flush_ready(disp);
        return;
    }

    esp_lcd_panel_draw_bitmap(s_panel, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, px_map);
    lv_display_flush_ready(disp);
}

/* -------------------- G5: táctil GT911 @0x14 (I2C SDA=GPIO7/SCL=GPIO8) -------------------- */
#define TOUCH_I2C_SDA   7
#define TOUCH_I2C_SCL   8
#define TOUCH_I2C_PORT  0

static esp_lcd_touch_handle_t s_tp = NULL;
static int32_t s_tp_x = 0, s_tp_y = 0;   /* último punto (LVGL lo quiere también en RELEASED) */

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
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &i2c_bus));

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

/* read_cb del lv_indev: sondea el GT911 y entrega el punto a LVGL (igual camino que
 * el táctil del STM32). LVGL lo llama dentro de lv_timer_handler. */
static void p4_lv_touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void) indev;
    uint16_t x = 0, y = 0;
    uint8_t  cnt = 0;
    esp_lcd_touch_read_data(s_tp);
    bool pressed = esp_lcd_touch_get_coordinates(s_tp, &x, &y, NULL, &cnt, 1);
    if (pressed && cnt > 0) {
        s_tp_x = x;
        s_tp_y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->point.x = s_tp_x;
    data->point.y = s_tp_y;
}

/* Clic del botón -> actualiza la etiqueta: prueba VISIBLE de que el toque llega. */
static void p4_btn_clicked(lv_event_t *e)
{
    lv_obj_t *label = (lv_obj_t *) lv_event_get_user_data(e);
    static int n = 0;
    n++;
    lv_label_set_text_fmt(label, "Tocado %d", n);
}

/* ===================== G6 — costura de display para BasicPlus =====================
 * Implementa el contrato bpvm_gui_disp_* (bpvm_gui.h) que gui.c llama tras lv_init().
 * Reaprovecha el panel (G3) + el táctil (G5). El tamaño físico lo fija el panel
 * (1024x600), NO el modelo (gui.c pasa 480x320 lógicos; la paridad es por árbol). */
void bpvm_gui_disp_init(int w, int h)
{
    (void) w; (void) h;
    p4_dsi_panel_init();
    p4_backlight_on();

    lv_tick_set_cb(p4_lv_tick_ms);
    s_lv_disp = lv_display_create(s_cfg->hres, s_cfg->vres);
    lv_display_set_flush_cb(s_lv_disp, p4_lv_flush);
    /* Orientacion: la nativa del panel; Gui.setRotation() la cambia en runtime (el giro
     * lo hace p4_lv_flush; el tactil se transforma solo en lv_indev). */
    lv_display_set_rotation(s_lv_disp, s_rotation);

    size_t buf_px   = (size_t) s_cfg->hres * 120;   /* draw buffer parcial en PSRAM */
    void  *draw_buf = heap_caps_malloc(buf_px * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (draw_buf == NULL) { ESP_LOGE(TAG, "sin PSRAM para el draw buffer de LVGL"); return; }
    lv_display_set_buffers(s_lv_disp, draw_buf, NULL, buf_px * sizeof(uint16_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    s_tp = p4_touch_init();
    if (s_tp != NULL) {
        lv_indev_t *indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, p4_lv_touch_read);
    } else {
        ESP_LOGW(TAG, "sin tactil: la GUI se vera pero no reaccionara");
    }
    ESP_LOGI(TAG, "GUI display listo: %s %dx%d, LVGL %d.%d.%d, LV_COLOR_DEPTH=%d",
             s_cfg->name, s_cfg->hres, s_cfg->vres, LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR,
             LVGL_VERSION_PATCH, (int) LV_COLOR_DEPTH);
}

void bpvm_gui_disp_pump(void)
{
    /* Una iteración del lazo LVGL. La llama Gui.run() (builtins.c) entre frames; ese
     * mismo lazo polea vm->poll_cb para el KILL. Cedemos SIEMPRE >=1 tick (a 100 Hz
     * pdMS_TO_TICKS(<10)=0 -> vTaskDelay(0) no cede a IDLE0 -> TWDT). */
    uint32_t idle_ms = lv_timer_handler();
    if (idle_ms > 50) idle_ms = 50;
    TickType_t ticks = pdMS_TO_TICKS(idle_ms);
    if (ticks == 0) ticks = 1;
    vTaskDelay(ticks);
}

int bpvm_gui_disp_is_open(void) { return 1; }   /* micro: corre hasta KILL/reset */

/* Rotación en runtime (Gui.setRotation): el flush YA gira por sw (portado de
 * esp32p4-ws) y el táctil se transforma solo (lv_indev) — basta cambiar la rotación
 * del display. deg llega VALIDADO de gui.c (0/90/180/270). Antes del display:
 * queda como orientación de arranque. */
void bpvm_gui_disp_set_rotation(int deg)
{
    switch (deg) {
        case 0:   s_rotation = LV_DISPLAY_ROTATION_0;   break;
        case 90:  s_rotation = LV_DISPLAY_ROTATION_90;  break;
        case 180: s_rotation = LV_DISPLAY_ROTATION_180; break;
        case 270: s_rotation = LV_DISPLAY_ROTATION_270; break;
        default:  return;
    }
    if (s_lv_disp != NULL) lv_display_set_rotation(s_lv_disp, s_rotation);
}

void p4_gfx_lvgl_test(void)
{
    p4_dsi_panel_init();
    p4_backlight_on();

    lv_init();
    lv_tick_set_cb(p4_lv_tick_ms);

    s_lv_disp = lv_display_create(s_cfg->hres, s_cfg->vres);
    lv_display_set_flush_cb(s_lv_disp, p4_lv_flush);

    /* Buffer de dibujo PARCIAL de LVGL en PSRAM (120 líneas). */
    size_t buf_px   = (size_t) s_cfg->hres * 120;
    void  *draw_buf = heap_caps_malloc(buf_px * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (draw_buf == NULL) { ESP_LOGE(TAG, "sin PSRAM para el draw buffer de LVGL"); return; }
    lv_display_set_buffers(s_lv_disp, draw_buf, NULL, buf_px * sizeof(uint16_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* UI: fondo azul + un botón centrado con etiqueta. */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x10384f), LV_PART_MAIN);
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 320, 110);
    lv_obj_center(btn);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "Tocame");
    lv_obj_center(lbl);
    lv_obj_add_event_cb(btn, p4_btn_clicked, LV_EVENT_CLICKED, lbl);  /* G5: clic -> etiqueta */

    /* G5 — táctil: GT911 -> lv_indev de puntero. LVGL enruta el toque al widget bajo
     * el dedo -> su evento CLICKED -> p4_btn_clicked. */
    s_tp = p4_touch_init();
    if (s_tp != NULL) {
        lv_indev_t *indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, p4_lv_touch_read);
    } else {
        ESP_LOGW(TAG, "sin tactil: el boton se mostrara pero no reaccionara");
    }

    ESP_LOGI(TAG, "G5: LVGL %d.%d.%d + boton TACTIL OK (LV_COLOR_DEPTH=%d). Bombeando...",
             LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH, (int) LV_COLOR_DEPTH);

    /* Bombeo de LVGL en este task (main); el wire corre en su propio task en
     * paralelo. G6 moverá esto a Gui.run() de BP. LVGL NO es thread-safe: por
     * ahora SOLO este task lo toca.
     *
     * OJO (causa del TWDT del 1er intento): vTaskDelay(pdMS_TO_TICKS(5)) con
     * FreeRTOS a 100 Hz = 0 ticks -> vTaskDelay(0) NO bloquea (solo cede a prio>=),
     * así que IDLE0 (prio 0) nunca corría en CPU0 y saltaba el watchdog. Hay que
     * ceder SIEMPRE >=1 tick. */
    for (;;) {
        uint32_t idle_ms = lv_timer_handler();   /* ms hasta el próximo timer LVGL */
        if (idle_ms > 50) idle_ms = 50;          /* cap (cubre LV_NO_TIMER_READY = UINT32_MAX) */
        TickType_t ticks = pdMS_TO_TICKS(idle_ms);
        if (ticks == 0) ticks = 1;               /* CLAVE: >=1 tick -> IDLE0 corre -> sin TWDT */
        vTaskDelay(ticks);
    }
}
