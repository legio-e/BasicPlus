/*
 * gui_display_ltdc.c — la pantalla de la DK2 (STM32 LTDC): V3/H5.1, y desde V7/H6 G5.b en DOS MODOS.
 *
 * Pensado para la STM32U5G9J-DK2: panel 800x480 RGB, capa LTDC 0, RGB565 (16bpp, ~750 KB de framebuffer en
 * SRAM interna), retroiluminación por PWM (TIM3_CH4) y táctil GT911 por el I2C 2.
 *
 *   BPVM_LVGL      LVGL va DENTRO de la imagen y este fichero le da su display y su táctil: el contrato
 *                  bpvm_gui_disp_* (bpvm_gui.h) que gui.c llama tras lv_init(). Lo de siempre.
 *   BPVM_GUI_PACK  (V7/H6 G5.b, 26-sep) LVGL viene de un PACK (lvgl.npk.ARMV8), como en el P4, la C5 y la C6, y
 *                  busca su driver de pantalla por la marca 'PANT'. El LTDC es un periférico de la familia y no se
 *                  presta por la BIOS, así que su driver se queda aquí y PUBLICA lo mismo que DriverSpi
 *                  (bpvm_pant.h): init, size, flush, rotate y touch.
 *
 * Lo común a los dos: el framebuffer (lo escanea el LTDC, así que es de la imagen en los dos modos), la luz y la
 * capa, y la lectura del GT911. Sin ninguno de los dos flags, la unidad de compilación queda vacía (la Nucleo).
 */
#if defined(BPVM_LVGL) || defined(BPVM_GUI_PACK)

#include "main.h"        /* HAL + handles hltdc / htim3 / hi2c2 (generados por CubeMX) */
#include "gui_display_ltdc.h"
#include <string.h>
#include <stdint.h>

#if defined(BPVM_LVGL)
#include "lvgl.h"
#include "bpvm_gui.h"
#include <stdio.h>        /* printf — aviso "setRotation no soportado en LTDC" */
#endif
#if defined(BPVM_GUI_PACK)
#include "bpvm_pant.h"    /* V7/H6 G5.b: el contrato del driver de pantalla ('PANT') */
#include "bpvm_bios.h"    /* V7/H6 G5.b: bpvm_bios_publica, el mismo registro que usan los packs */
#include "log.h"
#endif

/* Panel físico de la DK2. OJO: el tamaño "lógico" del modelo Gui (para el dumpTree / paridad miVM↔VM-C) es OTRO
 * — 480x320 en gui.c. Aquí manda el panel físico; como la paridad es por el árbol y NO por píxeles, decouplear el
 * tamaño físico del modelo es legítimo. */
#define LTDC_PANEL_W  800
#define LTDC_PANEL_H  480

/* Framebuffer RGB565 que escanea el LTDC (capa 0), en SRAM interna (~750 KB). */
static uint16_t s_framebuffer[LTDC_PANEL_W * LTDC_PANEL_H] __attribute__((aligned(32)));

extern LTDC_HandleTypeDef hltdc;   /* CubeMX MX_LTDC_Init */
extern TIM_HandleTypeDef  htim3;   /* CubeMX MX_TIM3_Init — CH4 = retro (BL_CTRL / PE6) */
extern I2C_HandleTypeDef  hi2c2;   /* CubeMX MX_I2C2_Init — bus del táctil GT911 (PF0/PF1) */

/* ===== Táctil GT911 (H5.2) =====
 * Datos del BSP de ST (stm32u5g9j_discovery_ts.h + gt911_reg.h): GT911 en I²C2, dirección 0xBA (8-bit), registros
 * de 16 bits. Secuencia de lectura: estado en 0x814E (bit7 = frame listo, bits 3:0 = nº de puntos); punto 1 en
 * 0x8150 (XL,XH,YL,YH → x=XH<<8|XL, y=YH<<8|YL); luego ACK escribiendo 0 en 0x814E para que el GT911 prepare el
 * siguiente frame. INT en PE5 (no se usa: se lee por sondeo, desde el lazo de la GUI). El punto sale en las
 * coordenadas del PANEL a 0°. */
#define GT911_ADDR       0xBAU
#define GT911_STAT_REG   0x814EU
#define GT911_P1_XL_REG  0x8150U

static int32_t s_tx = 0, s_ty = 0;   /* último punto (LVGL lo quiere también en RELEASED) */
static int     s_tpressed = 0;

/* Lee el GT911 si tiene un frame nuevo y devuelve el estado (1 = pulsado). Sin frame nuevo, el de antes: el GT911
 * sólo avisa cuando cambia algo. */
static int gt911_leer(void) {
    uint8_t st = 0;
    if (HAL_I2C_Mem_Read(&hi2c2, GT911_ADDR, GT911_STAT_REG, I2C_MEMADD_SIZE_16BIT,
                         &st, 1, 10) == HAL_OK && (st & 0x80U)) {
        if ((st & 0x0FU) >= 1U) {                      /* hay al menos un punto */
            uint8_t d[4];
            if (HAL_I2C_Mem_Read(&hi2c2, GT911_ADDR, GT911_P1_XL_REG,
                                 I2C_MEMADD_SIZE_16BIT, d, 4, 10) == HAL_OK) {
                s_tx = (int32_t) (d[0] | (d[1] << 8));
                s_ty = (int32_t) (d[2] | (d[3] << 8));
                s_tpressed = 1;
            }
        } else {
            s_tpressed = 0;
        }
        uint8_t zero = 0;                              /* ACK: prepara el siguiente frame */
        HAL_I2C_Mem_Write(&hi2c2, GT911_ADDR, GT911_STAT_REG, I2C_MEMADD_SIZE_16BIT,
                          &zero, 1, 10);
    }
    return s_tpressed;
}

/* La luz y la capa 0 del LTDC → nuestro framebuffer. Retroiluminación ON: CubeMX configuró el canal PWM pero NO lo
 * arranca; sin esto el panel queda negro aunque el LTDC escanee. (LCD_ON/PE4 ya está alto desde MX_GPIO_Init.) La
 * capa: CubeMX la dejó con FBStartAdress=0 / ImageWidth=0 (placeholder); se (re)configura entera aquí para no
 * depender de eso. HAL_LTDC_ConfigLayer habilita + recarga. */
static void ltdc_encender(void) {
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    LTDC_LayerCfgTypeDef lc = {0};
    lc.WindowX0 = 0; lc.WindowX1 = LTDC_PANEL_W;
    lc.WindowY0 = 0; lc.WindowY1 = LTDC_PANEL_H;
    lc.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    lc.Alpha = 255; lc.Alpha0 = 0;
    lc.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
    lc.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
    lc.FBStartAdress = (uint32_t) (uintptr_t) s_framebuffer;
    lc.ImageWidth = LTDC_PANEL_W; lc.ImageHeight = LTDC_PANEL_H;
    HAL_LTDC_ConfigLayer(&hltdc, &lc, 0);
}

#if defined(BPVM_LVGL)
/* ════════ LVGL DENTRO DE LA IMAGEN (lo de siempre) ════════ */

/* Buffer de dibujo PARCIAL de LVGL: renderiza las zonas sucias aquí y flush_cb las copia al framebuffer.
 * 48 líneas ≈ 77 KB (RGB565). */
#define DRAW_LINES 48
static uint8_t s_drawbuf[LTDC_PANEL_W * DRAW_LINES * 2] __attribute__((aligned(4)));

static void gt911_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    (void) indev;
    int pulsado = gt911_leer();
    data->point.x = s_tx;
    data->point.y = s_ty;
    data->state = pulsado ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/* Copia la zona renderizada (RGB565) al framebuffer, fila a fila. */
static void ltdc_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    int32_t w = area->x2 - area->x1 + 1;
    for (int32_t y = area->y1; y <= area->y2; y++) {
        memcpy(&s_framebuffer[(uint32_t) y * LTDC_PANEL_W + (uint32_t) area->x1],
               px_map, (size_t) w * 2u);          /* RGB565 = 2 bytes/px */
        px_map += w * 2;
    }
    lv_display_flush_ready(disp);
}

void bpvm_gui_disp_init(int w, int h) {
    (void) w; (void) h;   /* el tamaño físico lo fija el panel, no el modelo */
    ltdc_encender();

    /* Tick de LVGL = SysTick de la HAL (ms). */
    lv_tick_set_cb(HAL_GetTick);

    /* Display LVGL: tamaño del panel + flush_cb + buffer parcial. */
    lv_display_t* disp = lv_display_create(LTDC_PANEL_W, LTDC_PANEL_H);
    lv_display_set_flush_cb(disp, ltdc_flush_cb);
    lv_display_set_buffers(disp, s_drawbuf, NULL, sizeof(s_drawbuf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Táctil GT911 → indev de puntero. LVGL enruta el toque al widget bajo el dedo → su evento de clic →
     * lvgl_click_cb → bpvm_gui_inject_click → upcall onClick. Es EXACTAMENTE el mismo camino que el clic
     * sintético del host, ahora disparado por un dedo real sobre el panel. */
    lv_indev_t* touch = lv_indev_create();
    lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch, gt911_read_cb);
}

uint32_t bpvm_gui_disp_pump(void) {
    /* Procesa timers/render de LVGL y DEVUELVE el ocio que pide.
     *
     * #462 — aqui habia un __WFI(), que duerme el NUCLEO hasta la siguiente IRQ. No giraba al 100 % de CPU, que
     * era su motivo, pero paraba la tarea `vm` y con ella TODOS los hilos BP: el lazo del GUI se llevaba el turno
     * y nadie mas corria. Ahora el ocio lo duerme el lazo BP (`sleep` de BP), que cede el turno al siguiente hilo
     * BP; y cuando no queda ninguno ejecutable es el planificador quien duerme, por el mismo camino de siempre.
     *
     * lv_timer_handler se auto-regula por lv_tick (no re-renderiza de mas). */
    uint32_t idle_ms = lv_timer_handler();
    if (idle_ms > BPVM_GUI_OCIO_MAX_MS) idle_ms = BPVM_GUI_OCIO_MAX_MS;
    return idle_ms;
}

/* H5.1: sin "ventana que cerrar"; Gui.run() corre hasta reset. */
int bpvm_gui_disp_is_open(void) { return 1; }

/* V6/P2.3 — el screen del modelo mide lo que mide el panel (decisión de Eduardo, 4-sep-2026). La DK2 es LTDC a
 * 800×480 fijos: se contesta sin tocar el hardware (gui.c pregunta ANTES de disp_init). */
int bpvm_gui_disp_native_size(int* w, int* h) { *w = LTDC_PANEL_W; *h = LTDC_PANEL_H; return 1; }

/* Rotación en runtime (Gui.setRotation): el LTDC escanea un framebuffer fijo y este flush NO gira (con el LVGL del
 * pack sí: ver BPVM_GUI_PACK, abajo). Aviso una vez y no-op. */
void bpvm_gui_disp_set_rotation(int deg)
{
    (void) deg;
    static int warned = 0;
    if (!warned) { warned = 1; printf("[gui] setRotation: no soportado en este display (LTDC) todavia\n"); }
}
#endif /* BPVM_LVGL */

#if defined(BPVM_GUI_PACK)
/* ═════════ V7/H6 G5.b (26-sep) — LA PANTALLA DE LA DK2, PUBLICADA COMO 'PANT' DESDE LA IMAGEN ═════════
 * LVGL ya no va en la imagen: viene del pack (lvgl.npk.ARMV8) y busca su driver por la marca 'PANT', en su primer
 * uso. El formato no se publica: RGB565 tal cual, lo que escanea el LTDC.
 *
 * EL GIRO, que la DK2 no tenía (con LVGL en la imagen, setRotation avisaba y no hacía nada). El LTDC no gira por
 * hardware; el LVGL del pack pinta con el ancho y el alto cambiados y no sabe de giros, así que la zona, los píxeles
 * y el punto del táctil se giran aquí, con las cuentas de LVGL 9.2.2 que se portaron para el P4 (lv_display.c
 * lv_display_rotate_area, lv_draw_sw.c rotate90/180/270_rgb565, lv_indev.c indev_pointer_proc). Compuestas, dicen
 * dónde cae cada píxel LÓGICO (X,Y) en el panel a 0° (W×H = 800×480):
 *
 *      90°   (Y,       H-1-X)          180°   (W-1-X, H-1-Y)          270°   (W-1-Y, X)
 *
 * y como aquí el destino ES el framebuffer, cada píxel va directo a su sitio, sin la franja intermedia que el P4
 * necesitaba para su draw_bitmap. */
static int s_pant_rot = 0;        /* 0, 90, 180 o 270 */
static int s_pant_listo = 0;      /* la luz y la capa, UNA vez */

static int pant_init(void)
{
    if (!s_pant_listo) {
        ltdc_encender();
        s_pant_listo = 1;
    }
    return 0;
}

static int pant_size(int* w, int* h)
{
    *w = LTDC_PANEL_W;
    *h = LTDC_PANEL_H;
    return 1;
}

static int pant_rotate(int deg)
{
    if (deg != 0 && deg != 90 && deg != 180 && deg != 270) return -1;
    s_pant_rot = deg;
    return 0;                     /* girado: el pack de LVGL cambia su resolución y repinta */
}

static void pant_flush(int x1, int y1, int x2, int y2, const uint8_t* px)
{
    const int32_t W = LTDC_PANEL_W, H = LTDC_PANEL_H;
    /* La zona, en la orientación ACTUAL, tiene que caber: LVGL las recorta a su resolución, y una que no cupiera
     * se salta entera antes que escribir fuera del framebuffer. */
    const int32_t lw = (s_pant_rot == 90 || s_pant_rot == 270) ? H : W;
    const int32_t lh = (s_pant_rot == 90 || s_pant_rot == 270) ? W : H;
    if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= lw || y2 >= lh) return;
    const int32_t w = x2 - x1 + 1;
    const uint16_t* s = (const uint16_t*) (const void*) px;
    if (s_pant_rot == 0) {
        for (int32_t y = y1; y <= y2; y++) {
            memcpy(&s_framebuffer[y * W + x1], s, (size_t) w * 2u);
            s += w;
        }
        return;
    }
    for (int32_t Y = y1; Y <= y2; Y++) {
        uint16_t* d;
        int32_t   paso;
        if (s_pant_rot == 90)       { d = &s_framebuffer[(H - 1 - x1) * W + Y];           paso = -W; }
        else if (s_pant_rot == 180) { d = &s_framebuffer[(H - 1 - Y) * W + (W - 1 - x1)]; paso = -1; }
        else                        { d = &s_framebuffer[x1 * W + (W - 1 - Y)];           paso = W;  }
        for (int32_t x = 0; x < w; x++) { *d = *s++; d += paso; }
    }
}

static int pant_touch(int* x, int* y)
{
    if (!gt911_leer()) return 0;
    /* El punto: indev_pointer_proc de lv_indev.c —del panel a 0° a la orientación actual—, como en el P4. */
    int32_t px = s_tx, py = s_ty;
    const int32_t W = LTDC_PANEL_W, H = LTDC_PANEL_H;
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

void stm32_pant_publicar(void)
{
    /* Al arrancar, antes que ningún pack: el de LVGL la busca en su PRIMER USO (el primer Gui.Screen), así que el
     * orden da igual; y si un pack de driver quisiera publicar otra 'PANT', el registro se lo niega con nombre (-3)
     * en vez de pisar la de la imagen. */
    int r = bpvm_bios_publica(BPVM_PANT_MARCA, &s_pant_api);
    if (r != 0)
        log_printf("pantalla: la DK2 NO pudo publicar 'PANT' (publica = %d): la GUI del pack no tendra pantalla", r);
    else
        log_printf("pantalla: la DK2 publica 'PANT' desde la imagen (LTDC 800x480, tactil GT911, gira)");
}
#endif /* BPVM_GUI_PACK */

#endif /* BPVM_LVGL || BPVM_GUI_PACK */
