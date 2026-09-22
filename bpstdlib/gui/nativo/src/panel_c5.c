/* panel_c5.c — el PANEL de la Waveshare ESP32-C5-Touch-LCD-2.8, DENTRO del pack
 * (V7/H6 P3): ST7789 de 240×320 por SPI, táctil CST3530 por I2C, y el reset y el
 * backlight por un expansor CH32V003 también por I2C. Escrito contra la HAL que
 * presta la BIOS v5 (gpio/spi/i2c/reloj/sleep) y NO contra el IDF: por eso
 * cabe en un pack y correría igual en otro micro con los mismos pines.
 *
 * Es el quinto backend del contrato `bpvm_gui_disp_*` (host SDL, STM32 LTDC,
 * P4 MIPI-DSI, C6 ST7789 por esp_lcd) y el primero que vive fuera de la imagen.
 * La forma es la del C6: dos búferes parciales de DRAW_LINES líneas, RGB565
 * girado a big-endian antes de salir por el SPI, MADCTL para girar. Lo que
 * cambia y está leído del esquema y del BSP de Waveshare (22-sep):
 *
 *     SPI: SCK GPIO6 · MOSI GPIO7 · DC GPIO9 · CS GPIO10   (la TF comparte el bus: MISO 8, CS 23)
 *     I2C: SDA GPIO0 · SCL GPIO1  (bus 0)
 *       CH32V003 @0x24  reg 0x02 dirección (bit 1 = salida) · 0x03 salida · 0x05 PWM del backlight
 *                       EXIO0 = reset del táctil · EXIO1 = reset del panel · EXIO3 = amplificador
 *       CST3530  @0x58  registro de 32 bits big-endian; DATA 0xD0070000 (9 bytes); END 0xD00002AB
 *
 * ⚠️ El reset del panel y el backlight NO son un GPIO: van por I2C al CH32. Sin
 * el expansor la pantalla se queda negra aunque el SPI hable perfectamente.
 *
 * Lo que se mide en la C5 (V7_IDEAS §H6 2.3): la fachada SPI del ESP32 va SIN DMA
 * (64 B por transacción), así que un flush de 24 líneas son ~180 transacciones.
 * Primero que funcione; la palanca, si molesta, es la fachada, no este fichero. */
#include "bpvm_bios.h"
#include "bpvm_gui.h"
#include "lvgl.h"
#include "src/draw/sw/lv_draw_sw.h"    /* lv_draw_sw_rgb565_swap */
#include <stdint.h>

extern const bpvm_bios_t* g_bios;
static void di(const char* m) { if (g_bios && g_bios->log) g_bios->log(m); }

#define LCD_SPI_BUS   2        /* GPSPI2 en la numeración de Spi.Bus */
#define LCD_SCK       6
#define LCD_MOSI      7
#define LCD_DC        9
#define LCD_CS        10
#define LCD_HZ        (40 * 1000 * 1000)
#define LCD_W         240
#define LCD_H         320
#define DRAW_LINES    24
#define DRAW_BYTES    (LCD_W * DRAW_LINES * 2)

#define I2C_BUS       0
#define I2C_SDA       0
#define I2C_SCL       1
#define I2C_HZ        400000
#define CH32_ADDR     0x24
#define CH32_REG_DIR  0x02
#define CH32_REG_OUT  0x03
#define CH32_REG_PWM  0x05
#define CH32_TP_RST   0x01     /* EXIO0 */
#define CH32_LCD_RST  0x02     /* EXIO1 */
#define CH32_AMP      0x08     /* EXIO3 */
#define CST_ADDR      0x58

/* Los dos búferes de dibujo, ESTÁTICOS: van en la .bss del pack (el bloque de RAM
 * que el cargador le da). Sin DMA no importa dónde estén. */
static uint8_t s_buf1[DRAW_BYTES];
static uint8_t s_buf2[DRAW_BYTES];
static lv_display_t* s_disp = 0;
static lv_indev_t*   s_touch = 0;
static int           s_open = 0;
static int           s_touch_ok = 0;
static uint8_t       s_ch32_out = 0;

/* ── el expansor CH32V003 ─────────────────────────────────────────────────── */
static int ch32_escribe(uint8_t reg, uint8_t v) {
    uint8_t d[2] = { reg, v };
    return g_bios->i2c_write(I2C_BUS, CH32_ADDR, d, 2) == 2 ? 0 : -1;
}
static int ch32_salida(uint8_t v) { s_ch32_out = v; return ch32_escribe(CH32_REG_OUT, v); }

/* ── el ST7789: comandos con DC en bajo, datos con DC en alto ─────────────── */
static void lcd_cmd(uint8_t c) {
    g_bios->gpio_write(LCD_DC, 0); g_bios->gpio_write(LCD_CS, 0);
    g_bios->spi_write(LCD_SPI_BUS, &c, 1);
    g_bios->gpio_write(LCD_CS, 1);
}
static void lcd_dat(const uint8_t* d, size_t n) {
    g_bios->gpio_write(LCD_DC, 1); g_bios->gpio_write(LCD_CS, 0);
    g_bios->spi_write(LCD_SPI_BUS, d, n);
    g_bios->gpio_write(LCD_CS, 1);
}
static void lcd_cmd1(uint8_t c, uint8_t p) { lcd_cmd(c); lcd_dat(&p, 1); }
static void lcd_ventana(int x1, int y1, int x2, int y2) {
    uint8_t c[4] = { (uint8_t) (x1 >> 8), (uint8_t) x1, (uint8_t) (x2 >> 8), (uint8_t) x2 };
    uint8_t r[4] = { (uint8_t) (y1 >> 8), (uint8_t) y1, (uint8_t) (y2 >> 8), (uint8_t) y2 };
    lcd_cmd(0x2A); lcd_dat(c, 4);          /* CASET */
    lcd_cmd(0x2B); lcd_dat(r, 4);          /* RASET */
    lcd_cmd(0x2C);                         /* RAMWR */
}

static void c5_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = (uint32_t) (area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t) (area->y2 - area->y1 + 1);
    lv_draw_sw_rgb565_swap(px_map, w * h);      /* el ST7789 quiere RGB565 big-endian por SPI */
    lcd_ventana(area->x1, area->y1, area->x2, area->y2);
    lcd_dat(px_map, (size_t) (w * h * 2u));
    lv_display_flush_ready(disp);               /* síncrono: al volver del SPI ya está */
}

/* ── el táctil CST3530 ────────────────────────────────────────────────────── */
static int cst_lee(uint32_t reg, uint8_t* d, size_t n) {
    uint8_t a[4] = { (uint8_t) (reg >> 24), (uint8_t) (reg >> 16), (uint8_t) (reg >> 8), (uint8_t) reg };
    if (g_bios->i2c_write(I2C_BUS, CST_ADDR, a, 4) != 4) return -1;
    return g_bios->i2c_read(I2C_BUS, CST_ADDR, d, n) == (int) n ? 0 : -1;
}
static void cst_fin(void) {
    uint8_t a[4] = { 0xD0, 0x00, 0x02, 0xAB };  /* END READ */
    g_bios->i2c_write(I2C_BUS, CST_ADDR, a, 4);
}
static void c5_touch_read(lv_indev_t* indev, lv_indev_data_t* data) {
    (void) indev;
    uint8_t b[9];
    data->state = LV_INDEV_STATE_RELEASED;
    if (!s_touch_ok || cst_lee(0xD0070000u, b, 9) != 0) return;
    int pulsado = ((b[3] & 0x0F) != 0) && ((b[8] & 0xF0) != 0);
    cst_fin();
    if (!pulsado) return;
    int x = ((b[7] & 0x0F) << 8) | b[4];
    int y = ((b[7] & 0xF0) << 4) | b[5];
    if (x < 0 || x >= LCD_W || y < 0 || y >= LCD_H) return;
    data->point.x = (int32_t) x; data->point.y = (int32_t) y;
    data->state = LV_INDEV_STATE_PRESSED;
}

/* ── el contrato bpvm_gui_disp_* (bpvm_gui.h) ─────────────────────────────── */
static uint32_t c5_tick(void) { return g_bios->now_ms(); }

void bpvm_gui_disp_init(int w, int h) {
    (void) w; (void) h;   /* el tamaño físico lo fija el panel; gui.c pasa el lógico */
    if (!g_bios) return;
    lv_tick_set_cb(c5_tick);

    /* 1. Los pines, reclamados como DE PACK (H4: con nombre si no se puede). */
    if (g_bios->gpio_init(LCD_DC, 1) != 0 || g_bios->gpio_init(LCD_CS, 1) != 0) { di("gui: la C5 no da DC/CS (GPIO9/10)"); return; }
    g_bios->gpio_write(LCD_CS, 1);
    if (g_bios->spi_init(LCD_SPI_BUS, LCD_SCK, LCD_MOSI, -1, LCD_HZ, 0) != 0)   { di("gui: la C5 no da el SPI 2"); return; }
    if (g_bios->i2c_init(I2C_BUS, I2C_SDA, I2C_SCL, I2C_HZ) != 0)                { di("gui: la C5 no da el I2C 0"); return; }

    /* 2. El expansor: EXIO0/1/3 salidas; reset del panel 200 ms abajo, 200 arriba
     *    (la secuencia del BSP de Waveshare); el táctil, 10 y 10. */
    if (ch32_escribe(CH32_REG_DIR, 0xFF) != 0) { di("gui: el CH32V003 no contesta en I2C 0x24 (sin el, ni reset ni backlight)"); return; }
    ch32_salida(CH32_TP_RST);                 g_bios->sleep_ms(200);   /* LCD_RST abajo */
    ch32_salida(CH32_TP_RST | CH32_LCD_RST);  g_bios->sleep_ms(200);   /* LCD_RST arriba */
    ch32_salida(CH32_LCD_RST);                g_bios->sleep_ms(10);    /* TP_RST abajo */
    ch32_salida(CH32_TP_RST | CH32_LCD_RST);  g_bios->sleep_ms(10);
    s_touch_ok = 1;

    /* 3. El panel: la secuencia del driver ST7789 del IDF (SLPOUT, MADCTL RGB,
     *    COLMOD 16 bpp, RAMCTRL big-endian, INVON, DISPON). */
    lcd_cmd(0x11); g_bios->sleep_ms(100);     /* SLPOUT */
    lcd_cmd1(0x36, 0x00);                     /* MADCTL: RGB, sin giro */
    lcd_cmd1(0x3A, 0x55);                     /* COLMOD: RGB565 */
    { uint8_t rc[2] = { 0x00, 0xF0 }; lcd_cmd(0xB0); lcd_dat(rc, 2); }   /* RAMCTRL */
    lcd_cmd(0x21);                            /* INVON: los IPS van invertidos */
    lcd_cmd(0x29);                            /* DISPON */

    /* 4. LVGL: display + búferes + táctil. */
    s_disp = lv_display_create(LCD_W, LCD_H);
    lv_display_set_flush_cb(s_disp, c5_flush);
    lv_display_set_buffers(s_disp, s_buf1, s_buf2, DRAW_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
    s_touch = lv_indev_create();
    lv_indev_set_type(s_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_touch, c5_touch_read);

    /* 5. Backlight a tope (PWM del CH32, 0..255). */
    ch32_escribe(CH32_REG_PWM, 255);
    s_open = 1;
    di("gui: panel ST7789 240x320 por SPI (HAL v5), tactil CST3530, backlight por CH32 — listo");
}

uint32_t bpvm_gui_disp_pump(void) {
    uint32_t idle_ms = lv_timer_handler();
    if (idle_ms > BPVM_GUI_OCIO_MAX_MS) idle_ms = BPVM_GUI_OCIO_MAX_MS;
    return idle_ms;
}
int bpvm_gui_disp_is_open(void) { return s_open; }
int bpvm_gui_disp_native_size(int* w, int* h) { *w = LCD_W; *h = LCD_H; return 1; }

void bpvm_gui_disp_set_rotation(int deg) {
    /* MADCTL: MV = swap, MX/MY = mirror. La RAM del ST7789 es de 240×320, la
     * misma que el panel: sin gap en ninguna orientación. El táctil no se gira
     * aquí (V7: pendiente de ver el panel en marcha). */
    if (!s_open) return;
    uint8_t m = 0x00;
    switch (deg) {
        case 90:  m = 0x20 | 0x40; break;   /* MV|MX */
        case 180: m = 0x40 | 0x80; break;   /* MX|MY */
        case 270: m = 0x20 | 0x80; break;   /* MV|MY */
        default:  m = 0x00; break;
    }
    lcd_cmd1(0x36, m);
    if (s_disp) {
        /* Con MV el panel direcciona 320 columnas × 240 filas: LVGL tiene que pintar eso. */
        if (deg == 90 || deg == 270) lv_display_set_resolution(s_disp, LCD_H, LCD_W);
        else                         lv_display_set_resolution(s_disp, LCD_W, LCD_H);
        lv_obj_invalidate(lv_screen_active());
    }
}
