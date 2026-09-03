/*
 * gui_display_st7789.c — la cintura de pantalla del ESP32-C6-LCD-1.3 (V6/P2).
 *
 * Panel: ST7789V2, IPS 240×240, SPI de 4 hilos, sin táctil. Los pines salen del
 * esquemático de Waveshare (tabla «PIN-OUT», leída el 3-sep-2026):
 *
 *     MOSI GPIO6 · SCLK GPIO7 · CS GPIO14 · DC GPIO15 · RES GPIO21 · BL GPIO22 (MOSFET)
 *     (la microSD comparte el bus: MISO GPIO5, CS GPIO4; el LED WS2812B va en GPIO8)
 *
 * Es el cuarto backend del contrato bpvm_gui_disp_* (host SDL, STM32 LTDC, P4
 * MIPI-DSI) y el primero por SPI, que es lo que abre la GUI a las placas chicas:
 * no hay framebuffer completo — LVGL pinta en dos buffers parciales de 24 líneas
 * (2 × 11 520 B, RAM interna con DMA) y el flush los manda por esp_lcd; el panel
 * tiene su propia RAM de 240×320. El RGB565 viaja big-endian por SPI, así que el
 * flush gira los bytes (lv_draw_sw_rgb565_swap) antes de enviar y da el
 * flush_ready cuando el DMA termina (on_color_trans_done). El bombeo es el del
 * P4: lv_timer_handler + ceder al menos un tick, tope 10 ms (#424).
 *
 * Lo que aún no está: PWM del backlight (hoy encendido a tope por GPIO), la
 * rotación con sus offsets (el ST7789 de 240×240 vive en una RAM de 240×320 y
 * girar 180° exige un gap de 80), y el LED RGB (que en BasicPlus es Neopixel).
 */
#ifdef BPVM_LVGL

#include "bpvm_gui.h"
#include "lvgl.h"
#include "src/draw/sw/lv_draw_sw.h"    /* lv_draw_sw_rgb565_swap */
#include "esp_lcd_panel_io.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char* TAG = "c6_lcd";

#define LCD_HOST      SPI2_HOST
#define LCD_MOSI      6
#define LCD_SCLK      7
#define LCD_CS        14
#define LCD_DC        15
#define LCD_RST       21
#define LCD_BL        22
#define LCD_W         240
#define LCD_H         240
#define LCD_PCLK_HZ   (40 * 1000 * 1000)    /* el ST7789 admite ~60; 40 va sobrado y limpio */
#define DRAW_LINES    24
#define DRAW_BYTES    (LCD_W * DRAW_LINES * 2)

static esp_lcd_panel_handle_t s_panel = NULL;
static lv_display_t*          s_disp  = NULL;
static int                    s_open  = 0;

static uint32_t c6_tick_ms(void) { return (uint32_t) (esp_timer_get_time() / 1000); }

/* El DMA terminó de mandar un trozo: LVGL puede reutilizar ese buffer. */
static bool c6_trans_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t* ed, void* ctx) {
    (void) io; (void) ed;
    lv_display_flush_ready((lv_display_t*) ctx);
    return false;
}

static void c6_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    (void) disp;
    uint32_t w = (uint32_t) (area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t) (area->y2 - area->y1 + 1);
    lv_draw_sw_rgb565_swap(px_map, w * h);      /* el ST7789 quiere RGB565 big-endian por SPI */
    esp_lcd_panel_draw_bitmap(s_panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
    /* el flush_ready lo da c6_trans_done cuando el DMA termina */
}

static void c6_panel_init(lv_display_t* disp) {
    spi_bus_config_t bus = {
        .mosi_io_num = LCD_MOSI, .sclk_io_num = LCD_SCLK, .miso_io_num = -1,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = DRAW_BYTES,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = LCD_CS, .dc_gpio_num = LCD_DC, .spi_mode = 0, .pclk_hz = LCD_PCLK_HZ,
        .trans_queue_depth = 10, .on_color_trans_done = c6_trans_done, .user_ctx = disp,
        .lcd_cmd_bits = 8, .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) LCD_HOST, &io_cfg, &io));

    esp_lcd_panel_dev_config_t dev = {
        .reset_gpio_num = LCD_RST, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &dev, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));   /* los IPS ST7789 van invertidos */
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_panel, 0, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    /* Backlight: GPIO22 por MOSFET, en alto = encendido. PWM por LEDC, más adelante. */
    gpio_config_t bl = { .pin_bit_mask = 1ULL << LCD_BL, .mode = GPIO_MODE_OUTPUT };
    gpio_config(&bl);
    gpio_set_level(LCD_BL, 1);
}

/* ===================== el contrato bpvm_gui_disp_* (bpvm_gui.h) ===================== */

void bpvm_gui_disp_init(int w, int h) {
    (void) w; (void) h;   /* el tamaño físico lo fija el panel; gui.c pasa el lógico */
    lv_tick_set_cb(c6_tick_ms);
    s_disp = lv_display_create(LCD_W, LCD_H);
    lv_display_set_flush_cb(s_disp, c6_flush);
    c6_panel_init(s_disp);
    void* b1 = heap_caps_malloc(DRAW_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void* b2 = heap_caps_malloc(DRAW_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (b1 == NULL) { ESP_LOGE(TAG, "sin RAM interna para el draw buffer de LVGL"); return; }
    lv_display_set_buffers(s_disp, b1, b2, DRAW_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
    s_open = 1;
    ESP_LOGI(TAG, "GUI display listo: ST7789 %dx%d por SPI a %d MHz, buffers 2x%d B, LVGL %d.%d.%d, LV_COLOR_DEPTH=%d",
             LCD_W, LCD_H, LCD_PCLK_HZ / 1000000, DRAW_BYTES,
             LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH, (int) LV_COLOR_DEPTH);
}

void bpvm_gui_disp_pump(void) {
    /* Una vuelta del lazo LVGL, y ceder SIEMPRE al menos un tick (tope 10 ms, #424). */
    uint32_t idle_ms = lv_timer_handler();
    if (idle_ms > 10) idle_ms = 10;
    TickType_t ticks = pdMS_TO_TICKS(idle_ms);
    if (ticks == 0) ticks = 1;
    vTaskDelay(ticks);
}

int bpvm_gui_disp_is_open(void) { return s_open; }

void bpvm_gui_disp_set_rotation(int deg) {
    /* El giro lo hace el PANEL (MADCTL), que sale gratis. Para 90/180/270 el ST7789
     * de 240×240 necesita además un gap (su RAM es de 240×320); se ajusta cuando se
     * vea en la placa — hoy sólo 0° está comprobado. */
    if (s_panel == NULL) return;
    switch (deg) {
        case 90:  esp_lcd_panel_swap_xy(s_panel, true);  esp_lcd_panel_mirror(s_panel, true,  false); break;
        case 180: esp_lcd_panel_swap_xy(s_panel, false); esp_lcd_panel_mirror(s_panel, true,  true);  break;
        case 270: esp_lcd_panel_swap_xy(s_panel, true);  esp_lcd_panel_mirror(s_panel, false, true);  break;
        default:  esp_lcd_panel_swap_xy(s_panel, false); esp_lcd_panel_mirror(s_panel, false, false); break;
    }
    printf("[gui] setRotation(%d): por MADCTL del panel; los offsets de 90/180/270 están por comprobar\n", deg);
}

#endif /* BPVM_LVGL */
