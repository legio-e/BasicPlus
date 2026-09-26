/*
 * gui_display_ltdc.h — la pantalla de la DK2 (LTDC). Ver gui_display_ltdc.c.
 */
#ifndef GUI_DISPLAY_LTDC_H
#define GUI_DISPLAY_LTDC_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BPVM_GUI_PACK)
/* V7/H6 G5.b — publica la pantalla como 'PANT' (bpvm_pant.h) para el LVGL del pack. Se llama UNA vez al arrancar,
 * antes que ningún Run: el pack de LVGL la busca en su primer uso. */
void stm32_pant_publicar(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* GUI_DISPLAY_LTDC_H */
