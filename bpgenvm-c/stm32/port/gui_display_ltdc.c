/*
 * gui_display_ltdc.c — backend de display LVGL para el MICRO (STM32 LTDC), V3/H5.1.
 *
 * Implementa el contrato bpvm_gui_disp_* (bpvm_gui.h) que gui.c llama tras
 * lv_init(). Equivalente micro del backend SDL del host: en vez de una ventana,
 * un framebuffer en SRAM que el LTDC escanea al panel RGB de la placa. El render
 * de widgets (lv_label/lv_button/align/…) es PORTABLE y vive en gui.c.
 *
 * Pensado para la STM32U5G9J-DK2: panel 800x480 RGB, capa LTDC 0, RGB565 (16bpp,
 * ~750 KB de framebuffer en SRAM interna), retroiluminación por PWM (TIM3_CH4).
 * Requiere LV_COLOR_DEPTH 16 en el lv_conf.h del micro (RGB565).
 *
 * Todo bajo BPVM_LVGL → sin el flag, unidad de compilación vacía (la build lean
 * del micro, H5.0, no lo nota).
 */
#ifdef BPVM_LVGL

#include "lvgl.h"
#include "main.h"        /* HAL + handles hltdc / htim3 (generados por CubeMX) */
#include "bpvm_gui.h"
#include <string.h>
#include <stdint.h>

/* Panel físico de la DK2. OJO: el tamaño "lógico" del modelo Gui (para el
 * dumpTree / paridad miVM↔VM-C) es OTRO — 480x320 en gui.c. Aquí manda el panel
 * físico; como la paridad es por el árbol y NO por píxeles, decouplear el tamaño
 * físico del modelo es legítimo (el host pinta a 480x320, el micro a 800x480, y
 * el volcado del árbol es idéntico). */
#define LTDC_PANEL_W  800
#define LTDC_PANEL_H  480

/* Framebuffer RGB565 que escanea el LTDC (capa 0), en SRAM interna (~750 KB). */
static uint16_t s_framebuffer[LTDC_PANEL_W * LTDC_PANEL_H] __attribute__((aligned(32)));

/* Buffer de dibujo PARCIAL de LVGL: renderiza las zonas sucias aquí y flush_cb
 * las copia al framebuffer. 48 líneas ≈ 77 KB (RGB565). */
#define DRAW_LINES 48
static uint8_t s_drawbuf[LTDC_PANEL_W * DRAW_LINES * 2] __attribute__((aligned(4)));

extern LTDC_HandleTypeDef hltdc;   /* CubeMX MX_LTDC_Init */
extern TIM_HandleTypeDef  htim3;   /* CubeMX MX_TIM3_Init — CH4 = retro (BL_CTRL / PE6) */
extern I2C_HandleTypeDef  hi2c2;   /* CubeMX MX_I2C2_Init — bus del táctil GT911 (PF0/PF1) */

/* ===== Táctil GT911 (H5.2) =====
 * Datos del BSP de ST (stm32u5g9j_discovery_ts.h + gt911_reg.h): GT911 en I²C2,
 * dirección 0xBA (8-bit), registros de 16 bits. Secuencia de lectura: estado en
 * 0x814E (bit7 = frame listo, bits 3:0 = nº de puntos); punto 1 en 0x8150
 * (XL,XH,YL,YH → x=XH<<8|XL, y=YH<<8|YL); luego ACK escribiendo 0 en 0x814E para
 * que el GT911 prepare el siguiente frame. INT en PE5 (no se usa: leemos por
 * polling desde el read_cb que LVGL llama en lv_timer_handler). */
#define GT911_ADDR       0xBAU
#define GT911_STAT_REG   0x814EU
#define GT911_P1_XL_REG  0x8150U

static int32_t s_tx = 0, s_ty = 0;   /* último punto (LVGL lo quiere también en RELEASED) */
static int     s_tpressed = 0;

static void gt911_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    (void) indev;
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
    data->point.x = s_tx;
    data->point.y = s_ty;
    data->state = s_tpressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
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

    /* Retroiluminación ON: CubeMX configuró el canal PWM pero NO lo arranca; sin
     * esto el panel queda negro aunque el LTDC escanee. (LCD_ON/PE4 ya está alto
     * desde MX_GPIO_Init.) */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

    /* Capa 0 del LTDC → nuestro framebuffer, RGB565, pantalla completa. CubeMX la
     * dejó con FBStartAdress=0 / ImageWidth=0 (placeholder); la (re)configuramos
     * entera aquí para no depender de eso. HAL_LTDC_ConfigLayer habilita + recarga. */
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

    /* Tick de LVGL = SysTick de la HAL (ms). */
    lv_tick_set_cb(HAL_GetTick);

    /* Display LVGL: tamaño del panel + flush_cb + buffer parcial. */
    lv_display_t* disp = lv_display_create(LTDC_PANEL_W, LTDC_PANEL_H);
    lv_display_set_flush_cb(disp, ltdc_flush_cb);
    lv_display_set_buffers(disp, s_drawbuf, NULL, sizeof(s_drawbuf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Táctil GT911 → indev de puntero. LVGL enruta el toque al widget bajo el
     * dedo → su evento de clic → lvgl_click_cb → bpvm_gui_inject_click → upcall
     * onClick. Es EXACTAMENTE el mismo camino que el clic sintético del host,
     * ahora disparado por un dedo real sobre el panel. */
    lv_indev_t* touch = lv_indev_create();
    lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch, gt911_read_cb);
}

void bpvm_gui_disp_pump(void) {
    /* Procesa timers/render de LVGL y DUERME hasta la próxima IRQ (__WFI): el
     * SysTick (1 ms) marca el ritmo del lazo y la IRQ de RX despierta al instante
     * si llega un byte. El RX entra por IRQ→ring (stm32_wire.c, DK2), NO por
     * sondeo, así que el KILL durante Gui.run() no se pierde aunque durmamos entre
     * frames — y no giramos al 100% de CPU. lv_timer_handler se auto-regula por
     * lv_tick (no re-renderiza de más). */
    lv_timer_handler();
    __WFI();
}

/* H5.1: sin "ventana que cerrar"; Gui.run() corre hasta reset. El KILL por el
 * wire DURANTE Gui.run() (sondear entre pumps) es pulido de H5.2. */
int bpvm_gui_disp_is_open(void) { return 1; }

/* Rotación en runtime (Gui.setRotation): el LTDC escanea un framebuffer fijo y este
 * flush NO gira aún (se haría como en el P4-ws: lv_draw_sw_rotate en el flush).
 * Aviso una vez y no-op. */
void bpvm_gui_disp_set_rotation(int deg)
{
    (void) deg;
    static int warned = 0;
    if (!warned) { warned = 1; printf("[gui] setRotation: no soportado en este display (LTDC) todavia\n"); }
}

#endif /* BPVM_LVGL */
