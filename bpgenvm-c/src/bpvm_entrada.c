/*
 * bpvm_entrada.c — V8/H2, paso 5, la pieza 3 (12-nov): LA TABLA 'ENTR' DE LA IMAGEN (bpvm_entr.h): el teclado y el
 * ratón del SO, para el pack de la GUI. La publica quien trae un teclado o un ratón (hoy, la radio del Bluetooth de las
 * ESP, bt_esp32_publicar), una vez. Una imagen que no la publica deja la GUI del pack como antes: sin teclado ni ratón.
 */
#include "bpvm_entr.h"
#include "bpvm_bios.h"
#include "bpvm_bt.h"
#include "bpvm_raton.h"
#include "bpvm_teclado.h"

static const bpvm_pack_sym_t SIMBOLOS[] = {
    { "teclado_abre",   (bpvm_pack_fn_t) bpvm_teclado_lector_abre },
    { "teclado_lee",    (bpvm_pack_fn_t) bpvm_teclado_lector_lee },
    { "raton_abre",     (bpvm_pack_fn_t) bpvm_raton_lector_abre },
    { "raton_lee",      (bpvm_pack_fn_t) bpvm_raton_lector_lee },
    { "hay_teclado",    (bpvm_pack_fn_t) bpvm_bt_hay_teclado },
    { "hay_raton",      (bpvm_pack_fn_t) bpvm_bt_hay_raton },
    { "raton_pantalla", (bpvm_pack_fn_t) bpvm_raton_pantalla },
};

static const bpvm_pack_api_t TABLA = {
    BPVM_ENTR_MARCA, BPVM_ENTR_API_VER, sizeof SIMBOLOS / sizeof SIMBOLOS[0], SIMBOLOS
};

int bpvm_entrada_publicar(void) {
    int r = bpvm_bios_publica(BPVM_ENTR_MARCA, &TABLA);
    return r == -3 ? 0 : r;   /* -3: ya estaba (otro que también trae teclado o ratón) */
}
