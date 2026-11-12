/* entrada_pack.c — V8/H2, paso 5, la pieza 3 (12-nov): lo que gui_entrada.c PIDE del teclado y del ratón del SO,
 * servido desde la tabla 'ENTR' que publica la imagen (bpvm_entr.h), como pant_lvgl.c busca 'PANT'. gui_entrada.c es
 * el MISMO fichero del PC (bpgenvm-c/src): aquí sólo cambia quién está detrás de estos siete nombres.
 *
 * Sin 'ENTR' (una imagen de antes, o una placa sin teclado ni ratón), no hay entrada: pant_lvgl.c pregunta antes
 * (bpvm_entrada_pack_hay) y no la arranca, y la GUI se queda como antes. Se busca una vez: la imagen la publica al
 * arrancar, antes de que un programa cree su pantalla. */
#include "bpvm_bios.h"
#include "bpvm_bt.h"
#include "bpvm_entr.h"
#include "bpvm_raton.h"
#include "bpvm_teclado.h"
#include "lvgl.h"

extern const bpvm_bios_t* g_bios;

static bpvm_entr_t s_e;
static int         s_estado = 0;   /* 0 sin buscar · 1 resuelta · -1 no está o no vale */

int bpvm_entrada_pack_hay(void) {
    if (s_estado == 0) {
        const bpvm_pack_api_t* api = g_bios ? (const bpvm_pack_api_t*) g_bios->busca(BPVM_ENTR_MARCA) : 0;
        const char* mal = api ? bpvm_entr_resolver(api, &s_e) : "nadie la publica";
        s_estado = mal ? -1 : 1;
        if (g_bios && g_bios->log) {
            char b[120];
            lv_snprintf(b, sizeof b, "gui: el teclado y el raton, %s%s", mal ? "no: la entrada ('ENTR') " : "por 'ENTR'",
                        mal ? mal : "");
            g_bios->log(b);
        }
    }
    return s_estado > 0;
}

int bpvm_teclado_lector_abre(void) { return bpvm_entrada_pack_hay() ? s_e.teclado_abre() : -1; }

int bpvm_teclado_lector_lee(int id, int32_t* k, int32_t* cp) {
    return bpvm_entrada_pack_hay() ? s_e.teclado_lee(id, k, cp) : 0;
}

int bpvm_raton_lector_abre(void) { return bpvm_entrada_pack_hay() ? s_e.raton_abre() : -1; }

int bpvm_raton_lector_lee(int id, int32_t* x, int32_t* y, int* izq, int32_t* rueda) {
    return bpvm_entrada_pack_hay() ? s_e.raton_lee(id, x, y, izq, rueda) : 0;
}

int bpvm_bt_hay_teclado(void) { return bpvm_entrada_pack_hay() ? s_e.hay_teclado() : 0; }

int bpvm_bt_hay_raton(void) { return bpvm_entrada_pack_hay() ? s_e.hay_raton() : 0; }

void bpvm_raton_pantalla(int32_t w, int32_t h) {
    if (bpvm_entrada_pack_hay() && s_e.raton_pantalla) s_e.raton_pantalla(w, h);
}
