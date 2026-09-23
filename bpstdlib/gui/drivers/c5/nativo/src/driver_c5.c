/* driver_c5.c — el DRIVER DE PANTALLA de la Waveshare ESP32-C5-Touch-LCD-2.8, en
 * su propio pack (V7/H6.c G2; hasta entonces `panel_c5.c`, compilado DENTRO del
 * pack de LVGL): ST7789 de 240×320 por SPI, táctil CST3530 por I2C, y el reset y
 * el backlight por un expansor CH32V003 también por I2C. Escrito contra la HAL
 * que presta la BIOS (gpio/spi/i2c/reloj/sleep) y NO contra el IDF: por eso cabe
 * en un pack y correría igual en otro micro con los mismos pines.
 *
 * ─── LO QUE CAMBIÓ AL SACARLO DEL PACK DE LVGL ───
 *
 * No sabe NADA de LVGL. Publica 'PANT' (`bpvm_pant.h`) —init, size, flush,
 * rotate, touch, formato— y el pack de LVGL lo busca en su primer uso. El
 * display, los búferes de dibujo, el táctil como `lv_indev` y el giro de los
 * bytes se quedaron allí (`pant_lvgl.c`); aquí sólo queda el hardware. Pesa ~2 KB
 * de código: es lo que varía por placa, y ya no viaja dentro de lo que no varía.
 *
 * Es un driver DE PLACA (el «nivel 2» de V7_IDEAS §H6.7.4): los pines están
 * escritos aquí. Desde G3 (23-sep) la C5 funciona también SIN escribir C, con el
 * driver genérico (`../spi/`, `DriverSpi`) y su perfil en el ENV —y manda a la
 * pantalla lo mismo que éste, byte a byte: `make test-driverspi`—. Éste se queda
 * como EJEMPLO de driver de placa en C, para lo que no se pueda describir con
 * datos, y como la referencia contra la que se compara el genérico.
 *
 * Lo que está leído del esquema y del BSP de Waveshare (22-sep):
 *
 *     SPI: SCK GPIO6 · MOSI GPIO7 · DC GPIO9 · CS GPIO10   (la TF comparte el bus: MISO 8, CS 23)
 *     I2C: SDA GPIO0 · SCL GPIO1  (bus 0)
 *       CH32V003 @0x24  reg 0x02 dirección (bit 1 = salida) · 0x03 salida · 0x05 PWM del backlight
 *                       EXIO0 = reset del táctil · EXIO1 = reset del panel · EXIO3 = amplificador
 *       CST3530  @0x58  registro de 32 bits big-endian; DATA 0xD0070000 (9 bytes); END 0xD00002AB
 *
 * ⚠️ El reset del panel y el backlight NO son un GPIO: van por I2C al CH32. Sin
 * el expansor la pantalla se queda negra aunque el SPI hable perfectamente. */
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "bpvm_pant.h"
#include <stdint.h>
#include <stddef.h>

const bpvm_bios_t* g_bios = 0;
static void di(const char* m) { if (g_bios && g_bios->log) g_bios->log(m); }

#define LCD_SPI_BUS   2        /* GPSPI2 en la numeración de Spi.Bus */
#define LCD_SCK       6
#define LCD_MOSI      7
#define LCD_DC        9
#define LCD_CS        10
#define LCD_HZ        (40 * 1000 * 1000)
#define LCD_W         240
#define LCD_H         320

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

static int     s_touch_ok = 0;
static uint8_t s_ch32_out = 0;

/* ── el expansor CH32V003 ─────────────────────────────────────────────────── */
static int ch32_escribe(uint8_t reg, uint8_t v) {
    uint8_t d[2];
    d[0] = reg; d[1] = v;
    return g_bios->i2c_write(I2C_BUS, CH32_ADDR, d, 2) == 2 ? 0 : -1;
}
static int ch32_salida(uint8_t v) { s_ch32_out = v; return ch32_escribe(CH32_REG_OUT, v); }

/* ── el ST7789: comandos con DC en bajo, datos con DC en alto ─────────────── */
static void lcd_cmd(uint8_t c) {
    g_bios->gpio_write(LCD_DC, 0); g_bios->gpio_write(LCD_CS, 0);
    g_bios->spi_write(LCD_SPI_BUS, &c, 1);
    g_bios->gpio_write(LCD_CS, 1);
}
/* ⚠️ TROCEADO A `SPI_MAX`, y no es una precaución: la fachada SPI del ESP32 abre
 * el bus SIN DMA (`SPI_DMA_DISABLED`, `max_transfer_sz = 0`), así que el límite
 * de una transacción es el búfer del periférico —64 bytes—. Un flush de 24
 * líneas son 11 520 bytes: pasárselos de una vez no es «más lento», es una
 * transacción inválida. La palanca, si molesta, es la FACHADA (DMA), no este
 * fichero — V7_IDEAS §H6.7.11 y G4. */
#define SPI_MAX 64
static void lcd_dat(const uint8_t* d, size_t n) {
    g_bios->gpio_write(LCD_DC, 1); g_bios->gpio_write(LCD_CS, 0);
    while (n > 0) {
        size_t t = (n > SPI_MAX) ? SPI_MAX : n;
        g_bios->spi_write(LCD_SPI_BUS, d, t);
        d += t; n -= t;
    }
    g_bios->gpio_write(LCD_CS, 1);
}
static void lcd_cmd1(uint8_t c, uint8_t p) { lcd_cmd(c); lcd_dat(&p, 1); }
static void lcd_ventana(int x1, int y1, int x2, int y2) {
    uint8_t c[4], r[4];
    c[0] = (uint8_t) (x1 >> 8); c[1] = (uint8_t) x1; c[2] = (uint8_t) (x2 >> 8); c[3] = (uint8_t) x2;
    r[0] = (uint8_t) (y1 >> 8); r[1] = (uint8_t) y1; r[2] = (uint8_t) (y2 >> 8); r[3] = (uint8_t) y2;
    lcd_cmd(0x2A); lcd_dat(c, 4);          /* CASET */
    lcd_cmd(0x2B); lcd_dat(r, 4);          /* RASET */
    lcd_cmd(0x2C);                         /* RAMWR */
}

/* ── el táctil CST3530 ────────────────────────────────────────────────────── */
static int cst_lee(uint32_t reg, uint8_t* d, size_t n) {
    uint8_t a[4];
    a[0] = (uint8_t) (reg >> 24); a[1] = (uint8_t) (reg >> 16); a[2] = (uint8_t) (reg >> 8); a[3] = (uint8_t) reg;
    if (g_bios->i2c_write(I2C_BUS, CST_ADDR, a, 4) != 4) return -1;
    return g_bios->i2c_read(I2C_BUS, CST_ADDR, d, n) == (int) n ? 0 : -1;
}
static void cst_fin(void) {
    uint8_t a[4];
    a[0] = 0xD0; a[1] = 0x00; a[2] = 0x02; a[3] = 0xAB;   /* END READ */
    g_bios->i2c_write(I2C_BUS, CST_ADDR, a, 4);
}

/* ── LO QUE SE PUBLICA BAJO 'PANT' ─────────────────────────────────────────── */

static int c5_size(int* w, int* h) { *w = LCD_W; *h = LCD_H; return 1; }

/* El ST7789 por SPI quiere el byte alto primero: se lo pide a LVGL, que gira los
 * bytes con su rutina antes del flush. Aquí sólo se mandan. */
static int c5_formato(void) { return BPVM_PANT_RGB565_BE; }

static int c5_init(void) {
    if (!g_bios) return -1;
    /* ⚠️ MIGAS DE PAN, y no son andamio: aquí dentro no hay depurador, y el
     * 22-sep el primer `Gui.screen()` en la C5 reinició la placa sin dejar NI UNA
     * línea del pack — con lo que no se podía distinguir «se cayó en el SPI» de
     * «se cayó antes de empezar». Cada paso se cuenta ANTES de darlo. */
    di("driver C5 — entrando");

    /* 1. Los pines, reclamados como DE PACK (H4: con nombre si no se puede). */
    if (g_bios->gpio_init(LCD_DC, 1) != 0 || g_bios->gpio_init(LCD_CS, 1) != 0) { di("driver C5: la placa no da DC/CS (GPIO9/10)"); return -1; }
    g_bios->gpio_write(LCD_CS, 1);
    if (g_bios->spi_init(LCD_SPI_BUS, LCD_SCK, LCD_MOSI, -1, LCD_HZ, 0) != 0)   { di("driver C5: la placa no da el SPI 2"); return -2; }
    if (g_bios->i2c_init(I2C_BUS, I2C_SDA, I2C_SCL, I2C_HZ) != 0)                { di("driver C5: la placa no da el I2C 0"); return -3; }
    di("driver C5 — pines, SPI2 e I2C0 tomados");

    /* 2. El expansor: EXIO0/1/3 salidas; reset del panel 200 ms abajo, 200 arriba
     *    (la secuencia del BSP de Waveshare); el táctil, 10 y 10. */
    if (ch32_escribe(CH32_REG_DIR, 0xFF) != 0) { di("driver C5: el CH32V003 no contesta en I2C 0x24 (sin el, ni reset ni backlight)"); return -4; }
    ch32_salida(CH32_TP_RST);                 g_bios->sleep_ms(200);   /* LCD_RST abajo */
    ch32_salida(CH32_TP_RST | CH32_LCD_RST);  g_bios->sleep_ms(200);   /* LCD_RST arriba */
    ch32_salida(CH32_LCD_RST);                g_bios->sleep_ms(10);    /* TP_RST abajo */
    ch32_salida(CH32_TP_RST | CH32_LCD_RST);  g_bios->sleep_ms(10);
    s_touch_ok = 1;
    di("driver C5 — CH32V003 ok, resets del panel y del tactil dados");

    /* 3. El panel: la secuencia del driver ST7789 del IDF (SLPOUT, MADCTL RGB,
     *    COLMOD 16 bpp, RAMCTRL big-endian, INVON, DISPON). El orden RGB/BGR de
     *    otra placa sería el bit 3 de MADCTL: lo hace el chip, gratis. */
    lcd_cmd(0x11); g_bios->sleep_ms(100);     /* SLPOUT */
    lcd_cmd1(0x36, 0x00);                     /* MADCTL: RGB, sin giro */
    lcd_cmd1(0x3A, 0x55);                     /* COLMOD: RGB565 */
    { uint8_t rc[2]; rc[0] = 0x00; rc[1] = 0xF0; lcd_cmd(0xB0); lcd_dat(rc, 2); }   /* RAMCTRL */
    lcd_cmd(0x21);                            /* INVON: los IPS van invertidos */
    lcd_cmd(0x29);                            /* DISPON */
    di("driver C5 — ST7789 inicializado (SLPOUT/MADCTL/COLMOD/RAMCTRL/INVON/DISPON)");

    /* 4. Backlight a tope (PWM del CH32, 0..255). */
    ch32_escribe(CH32_REG_PWM, 255);
    di("driver C5: ST7789 240x320 por SPI, tactil CST3530, backlight por CH32 — listo");
    return 0;
}

/* La zona ya viene en el orden de bytes que pedimos (`formato`). */
static void c5_flush(int x1, int y1, int x2, int y2, const uint8_t* px) {
    lcd_ventana(x1, y1, x2, y2);
    lcd_dat(px, (size_t) (x2 - x1 + 1) * (size_t) (y2 - y1 + 1) * 2u);
}

static int c5_rotate(int deg) {
    /* MADCTL: MV = swap, MX/MY = mirror. La RAM del ST7789 es de 240×320, la
     * misma que el panel: sin gap en ninguna orientación. El táctil no se gira
     * (pendiente desde H6.a: sus coordenadas son las de 0°). */
    uint8_t m;
    switch (deg) {
        case 0:   m = 0x00; break;
        case 90:  m = 0x20 | 0x40; break;   /* MV|MX */
        case 180: m = 0x40 | 0x80; break;   /* MX|MY */
        case 270: m = 0x20 | 0x80; break;   /* MV|MY */
        default:  return -1;
    }
    lcd_cmd1(0x36, m);
    return 0;
}

static int c5_touch(int* x, int* y) {
    uint8_t b[9];
    if (!s_touch_ok || cst_lee(0xD0070000u, b, 9) != 0) return 0;
    int pulsado = ((b[3] & 0x0F) != 0) && ((b[8] & 0xF0) != 0);
    cst_fin();
    if (!pulsado) return 0;
    int tx = ((b[7] & 0x0F) << 8) | b[4];
    int ty = ((b[7] & 0xF0) << 4) | b[5];
    if (tx < 0 || tx >= LCD_W || ty < 0 || ty >= LCD_H) return 0;
    *x = tx; *y = ty;
    return 1;
}

static const bpvm_pack_sym_t s_syms[] = {
    { "init",    (bpvm_pack_fn_t) c5_init    },
    { "size",    (bpvm_pack_fn_t) c5_size    },
    { "flush",   (bpvm_pack_fn_t) c5_flush   },
    { "rotate",  (bpvm_pack_fn_t) c5_rotate  },
    { "touch",   (bpvm_pack_fn_t) c5_touch   },
    { "formato", (bpvm_pack_fn_t) c5_formato },
};
static const bpvm_pack_api_t s_api = {
    BPVM_PANT_MARCA, BPVM_PANT_API_VER,
    (uint32_t) (sizeof s_syms / sizeof s_syms[0]), s_syms
};

/* La ENTRADA. Sólo publica: la pantalla se enciende cuando el pack de LVGL la
 * pide (`init`), no al cargar — un programa sin GUI no se queda sin los pines. */
int32_t bp_pack_init(const bpvm_bios_t* bios)
{
    if (bios == 0)                          return 1;
    if (bios->magic   != BPVM_BIOS_MAGIC)   return 2;
    if (bios->version <  5u)                return 3;   /* la HAL es de la v5; la tabla crece por el final */
    if (bios->log     == 0)                 return 4;
    g_bios = bios;
    int r = bios->publica(BPVM_PANT_MARCA, &s_api);
    if (r != 0) {
        di(r == -3 ? "driver C5: 'PANT' ya estaba publicada (¿dos drivers de pantalla grabados?)"
         : r == -4 ? "driver C5: la BIOS RECHAZO la tabla (la reja)"
                   : "driver C5: publica('PANT') fallo");
        return 5;
    }
    di("driver C5: publicado como 'PANT' (ST7789 240x320 + CST3530 + CH32V003)");
    return 0;
}
