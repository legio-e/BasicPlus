/*
 * bpvm_gui.h — backend GUI (modelo de comportamiento) de la VM-C, V3 / H4.1.
 *
 * Espejo del GuiBackend de miVM: cada widget BP es un `handle` (int) que mapea
 * a un nodo con (type, text, w, h, align, dx, dy, parent, objptr). El volcado
 * dump_tree produce el MISMO texto byte-a-byte que miVM → paridad de
 * comportamiento (que se mide sobre el árbol del UI, NO sobre píxeles).
 *
 * H4.1 es SÓLO el modelo: basta para la paridad y no arrastra dependencias.
 * El render real (LVGL + SDL en host) cuelga de este modelo en H4.2. Los
 * builtins __gui* (builtins.c) delegan aquí; el upcall onClick lo dispara el
 * case GUI_RUN via bpvm_call_bp_from_builtin (interp.c).
 */
#ifndef BPVM_GUI_H
#define BPVM_GUI_H

#include <stddef.h>
#include <stdint.h>

/* Creación / pantalla raíz. Devuelven el handle (>0), 0 si overflow. */
int  bpvm_gui_screen_active(void);
int  bpvm_gui_create_obj(int parent);
int  bpvm_gui_create_label(int parent);
int  bpvm_gui_create_button(int parent);

/* Configuración (sólo lo que afecta al dumpTree: texto + geometría/anclaje).
 * Color y fuente son render-only → no tienen entrada aquí (no-op en H4.1). */
void bpvm_gui_set_text(int handle, const char* s);
void bpvm_gui_set_width(int handle, int w);
void bpvm_gui_set_height(int handle, int h);
void bpvm_gui_align(int handle, int a, int dx, int dy);
void bpvm_gui_clean(int handle);
void bpvm_gui_delete(int handle);

/* Eventos. bind asocia el objeto BP dueño del widget; inject encola un clic
 * sintético (diagnóstico / pruebas headless); next saca el siguiente objptr
 * (0 = cola vacía) — lo drena el bombeo de GUI_RUN. */
void     bpvm_gui_bind_click(int handle, uint32_t objptr);
void     bpvm_gui_inject_click(uint32_t objptr);
uint32_t bpvm_gui_next_click(void);

/* Vuelca el árbol de widgets a un buffer recién malloc'd (el caller hace free).
 * Devuelve la longitud en bytes (sin el '\0'). Byte-idéntico a miVM. */
size_t bpvm_gui_dump_tree(char** out);

#endif /* BPVM_GUI_H */
