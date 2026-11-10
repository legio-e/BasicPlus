/*
 * gui_entrada.h — V8/H2, paso 4 (10-nov): el teclado y el ratón del SO, en LVGL (gui_entrada.c; V8_IDEAS §E1, «El paso
 * 4»). Lo llama quien crea la pantalla de LVGL, después de la táctil.
 */
#ifndef GUI_ENTRADA_H
#define GUI_ENTRADA_H

#ifdef BPVM_LVGL
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Crea el teclado y el ratón de la GUI. `pc`: en el PC la táctil es el mismo ratón, así que con un ratón conectado se
 * apaga (si no, cada clic contaría dos veces); en una placa van los dos. `cursor_sistema`: esconde (0) o enseña (1) el
 * cursor del sistema en la ventana mientras se ve la flecha de la GUI (en el PC; NULL en una placa).
 * ⚠️ EN EL PC, ANTES QUE LOS DISPOSITIVOS DE SDL DE LVGL: sus manejadores (lv_sdl_keyboard_handler, lv_sdl_mouse_handler)
 * cogen el primero de cada tipo de la lista de LVGL, que empieza por el más nuevo (lv_indev_create inserta a la cabeza),
 * y dan por hecho que es el suyo: con los nuestros más nuevos, el teclado reventaba y la táctil se quedaba sin eventos. */
void bpvm_gui_entrada_init(int pc, void (*cursor_sistema)(int visible));

/* La táctil (el puntero que se apaga en el PC con un ratón conectado), ya creada. Comprueba que los manejadores de SDL
 * vayan a encontrar los suyos, y si no, lo dice. */
void bpvm_gui_entrada_tactil(lv_indev_t* tactil);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_LVGL */
#endif
