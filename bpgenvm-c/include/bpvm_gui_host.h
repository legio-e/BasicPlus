/*
 * bpvm_gui_host.h — lo de la GUI que SÓLO tiene el PC: la ventana (SDL).
 *
 * V7 #646 (7-oct): estos tres verbos vivían en la cabecera común del display (bpvm_gui.h), que es la costura con los
 * micros, y ningún micro los tiene —el generador de la costura, scripts/gen_gui_pack.py, les escribía funciones
 * vacías—. Los define src/gui_display_sdl.c y los llaman el ejecutable del PC (test/main.c) y el micro simulado
 * (tools/bpvm_sim.c), los dos con LVGL (BPVM_LVGL).
 */
#ifndef BPVM_GUI_HOST_H
#define BPVM_GUI_HOST_H

/* H10 — micro simulado SIN pantalla: LVGL se monta contra un display fuera de pantalla (los widgets se crean y el
 * modelo funciona; no se abre ventana y disp_is_open() da 0, así que Gui.run() vuelve en seguida). gui.c nunca la
 * invoca. En un build sin LVGL esto ya es lo que pasa. */
void bpvm_gui_disp_set_headless(int on);

/* Simulador: cerrar la ventana termina el proceso (ver gui_display_sdl.c). */
void bpvm_gui_disp_set_exit_on_close(int on);

/* #322 — título de la ventana. Sin llamarla, LVGL pone el suyo ("LVGL Simulator"), que dice qué librería dibuja en vez
 * de qué está corriendo. Llamar ANTES de disp_init. */
void bpvm_gui_disp_set_title(const char* title);

/* #813 (11-nov) — al revés que las de arriba: la define gui.c y la llama la ventana. El tamaño de la pantalla del
 * modelo, ya girado: en el PC, la ventana gira con la pantalla y mide siempre lo que ella. */
void bpvm_gui_get_screen_size(int* w, int* h);

#endif /* BPVM_GUI_HOST_H */
