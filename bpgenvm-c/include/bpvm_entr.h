/* bpvm_entr.h — V8/H2, paso 5, la pieza 3 (12-nov): EL CONTRATO DE LA ENTRADA, lo que publica bajo 'ENTR' quien trae
 * un teclado o un ratón, para el pack de la GUI, que lo busca como busca la táctil en 'PANT' (bpvm_pant.h). Es la API de
 * la idea del 14-oct (V8_IDEAS §E1): habla de teclado y de ratón, no de quién los trae. Hoy la publica la radio del
 * Bluetooth de las ESP (bt_esp32_publicar); un día, un USB, con la misma tabla.
 *
 * Lo que se publica es el teclado y el ratón DEL SO (bpvm_teclado.h, bpvm_raton.h): sus lectores de C, los del paso 4,
 * que en el PC lee gui_entrada.c directamente y en la placa, desde el pack, por aquí.
 *
 * ─── LOS NOMBRES (por nombre, como 'PANT': una opcional nueva no sube la versión) ───
 *
 *   obligatorias
 *     "teclado_abre"    int  teclado_abre(void)                 un lector del teclado del SO: su número, o -1
 *     "teclado_lee"     int  teclado_lee(int id, int32_t* k, int32_t* cp)
 *                            1 y la siguiente tecla (el int32 y su carácter), o 0 si no hay
 *     "raton_abre"      int  raton_abre(void)                   un lector del ratón del SO: su número, o -1
 *     "raton_lee"       int  raton_lee(int id, int32_t* x, int32_t* y, int* izq, int32_t* rueda)
 *                            dónde está, si el izquierdo está o estuvo apretado y las muescas de la rueda; 1, o 0
 *     "hay_teclado"     int  hay_teclado(void)                  ¿hay un teclado conectado?
 *     "hay_raton"       int  hay_raton(void)                    ¿hay un ratón conectado?
 *
 *   opcionales
 *     "raton_pantalla"  void raton_pantalla(int32_t w, int32_t h)
 *                            el tamaño de la pantalla tal como se ve, para que el ratón no se salga (al crearla y al
 *                            girarla: en la placa gira el driver, y la GUI lo sabe)
 */
#ifndef BPVM_ENTR_H
#define BPVM_ENTR_H

#include <stdint.h>
#include "bpvm_pack_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_ENTR_MARCA    0x454E5452u   /* 'ENTR' */
#define BPVM_ENTR_API_VER  1u

/* La tabla ya resuelta. La opcional puede ser NULL. */
typedef struct {
    int  (*teclado_abre)(void);
    int  (*teclado_lee)(int id, int32_t* k, int32_t* cp);
    int  (*raton_abre)(void);
    int  (*raton_lee)(int id, int32_t* x, int32_t* y, int* izq, int32_t* rueda);
    int  (*hay_teclado)(void);
    int  (*hay_raton)(void);
    void (*raton_pantalla)(int32_t w, int32_t h);
} bpvm_entr_t;

/* Resuelve una tabla publicada bajo 'ENTR' en `e`: NULL si vale; si no, el motivo, con el nombre de lo que falta.
 * `static inline`, como bpvm_pant_resolver: el que recoge la tabla se lleva el verificador. */
static inline const char* bpvm_entr_resolver(const bpvm_pack_api_t* api, bpvm_entr_t* e)
{
    if (api == 0 || e == 0) return "no hay tabla";
    {
        const char* mal = bpvm_pack_api_verify(api, BPVM_ENTR_MARCA, BPVM_ENTR_API_VER);
        if (mal) return mal;
    }
    e->teclado_abre   = (int  (*)(void))                                      bpvm_pack_api_find(api, "teclado_abre");
    e->teclado_lee    = (int  (*)(int, int32_t*, int32_t*))                   bpvm_pack_api_find(api, "teclado_lee");
    e->raton_abre     = (int  (*)(void))                                      bpvm_pack_api_find(api, "raton_abre");
    e->raton_lee      = (int  (*)(int, int32_t*, int32_t*, int*, int32_t*))   bpvm_pack_api_find(api, "raton_lee");
    e->hay_teclado    = (int  (*)(void))                                      bpvm_pack_api_find(api, "hay_teclado");
    e->hay_raton      = (int  (*)(void))                                      bpvm_pack_api_find(api, "hay_raton");
    e->raton_pantalla = (void (*)(int32_t, int32_t))                          bpvm_pack_api_find(api, "raton_pantalla");
    if (e->teclado_abre == 0) return "a la entrada le falta 'teclado_abre'";
    if (e->teclado_lee  == 0) return "a la entrada le falta 'teclado_lee'";
    if (e->raton_abre   == 0) return "a la entrada le falta 'raton_abre'";
    if (e->raton_lee    == 0) return "a la entrada le falta 'raton_lee'";
    if (e->hay_teclado  == 0) return "a la entrada le falta 'hay_teclado'";
    if (e->hay_raton    == 0) return "a la entrada le falta 'hay_raton'";
    return 0;
}

/* LA IMAGEN (bpvm_entrada.c): publica su tabla, con el teclado y el ratón del SO. Lo llama quien los trae (la radio del
 * Bluetooth), una vez; si ya estaba publicada (otro que también trae teclado o ratón), vale igual. 0, o el error de la
 * BIOS. */
int bpvm_entrada_publicar(void);

#ifdef __cplusplus
}
#endif
#endif
