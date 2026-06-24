/* gui_display_dsi.h — backend de display del ESP32-P4 (MIPI-DSI EK79007).
 *
 * G3: prueba de panel (init DSI + backlight + relleno) en NUESTRO firmware.
 * G4+: implementará la costura bpvm_gui_disp_* (bajo BPVM_LVGL) reutilizando el
 * init de panel de aquí. */
#ifndef GUI_DISPLAY_DSI_H
#define GUI_DISPLAY_DSI_H

/* G3 — inicializa el panel + backlight y pinta la pantalla de ROJO.
 * Si se ve roja: DSI + EK79007 + backlight + PSRAM OK en nuestro firmware. */
void p4_gfx_smoke_test(void);

#endif /* GUI_DISPLAY_DSI_H */
