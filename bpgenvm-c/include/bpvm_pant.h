/* bpvm_pant.h — el contrato del DRIVER DE PANTALLA: lo que publica bajo 'PANT'
 * (V7/H6.c G2).
 *
 * LVGL no sabe de pantallas: quiere volcar una zona de píxeles, saber el tamaño,
 * girar y leer el táctil. El driver sabe de SU hardware —el controlador (ST7789,
 * ILI9341…), el bus, los pines, cómo se hace el reset y cómo se enciende la luz— y
 * de nada más. Este fichero es la frontera entre los dos, y es pequeña a propósito.
 *
 * ─── QUIÉN LO PUBLICA Y QUIÉN LO USA ───
 *
 * Lo PUBLICA un driver de pantalla con `bios->publica(BPVM_PANT_MARCA, &tabla)`:
 * hoy un pack propio por placa (`bpstdlib/gui/drivers/<placa>/`), mañana también
 * la imagen (el P4 por MIPI-DSI y la DK2 por LTDC, `G5`: los periféricos de una
 * familia no se prestan por la BIOS, así que su driver se queda dentro y publica
 * lo mismo). Lo USA el pack de LVGL (`pant_lvgl.c`), que lo busca en su PRIMER
 * USO y no al cargar: así el orden en que se graben los dos packs no importa.
 *
 * ─── POR NOMBRE, NO POR POSICIÓN ───
 *
 * La tabla es una `bpvm_pack_api_t` —nombre → función— como la de 'LVGL' y la de
 * 'SQLI', y por lo mismo: el driver se compila aparte y se congela, y un nombre que
 * falte se DICE en vez de llamar a otra cosa. Y da una segunda ventaja que aquí se
 * usa: las funciones OPCIONALES son sencillamente las que un driver no publica
 * (uno sin táctil no publica `touch`), y añadir una en una versión futura no rompe
 * a los drivers ya grabados.
 *
 * ─── LOS NOMBRES ───
 *
 *   obligatorias
 *     "init"     int  init(void)
 *                Enciende la pantalla: reset, secuencia de arranque del
 *                controlador, luz. 0 = bien. La llama el pack de LVGL UNA vez, al
 *                montar el display (el primer `Gui.Screen()`), no al cargar el pack:
 *                un programa que no use la GUI no se queda sin los pines.
 *     "size"     int  size(int* w, int* h)
 *                El tamaño FÍSICO, en la orientación de 0°. 1 = lo sabe.
 *                ⚠️ Tiene que contestar ANTES de `init`: `gui.c` pregunta el tamaño
 *                antes de crear el screen («la resolución es la que tenga la
 *                pantalla, no hay otra» — Eduardo, 4-sep).
 *     "flush"    void flush(int x1, int y1, int x2, int y2, const uint8_t* px)
 *                Vuelca la zona [x1..x2]×[y1..y2] (inclusiva, en la orientación
 *                ACTUAL) con los píxeles en el formato que el driver pidió
 *                (`formato`), fila a fila. SÍNCRONO: al volver, el búfer es otra
 *                vez de LVGL.
 *
 *   opcionales
 *     "rotate"   int  rotate(int deg)
 *                0, 90, 180 o 270. 0 = girado (el pack de LVGL cambia entonces su
 *                resolución); -1 = no sabe girar, y la resolución no se toca. Sin
 *                publicar = no sabe girar.
 *     "touch"    int  touch(int* x, int* y)
 *                1 = pulsado, con el punto en coordenadas de la pantalla; 0 = no.
 *                Sin publicar = la pantalla no tiene táctil.
 *     "formato"  int  formato(void)
 *                Cómo quiere los píxeles (BPVM_PANT_RGB565*): LVGL pinta en ESE
 *                formato y el driver sólo manda bytes. Sin publicar = RGB565 tal
 *                cual (lo que quieren un LTDC o un MIPI-DSI).
 *
 * El orden RGB/BGR NO está aquí, a propósito: en los controladores por SPI es un
 * bit de MADCTL (lo hace el chip, gratis) y en un bus paralelo lo hace el
 * periférico. Es cosa del driver, y LVGL pinta igual. (Pregunta de Eduardo, 23-sep.)
 */
#ifndef BPVM_PANT_H
#define BPVM_PANT_H

#include <stdint.h>
#include "bpvm_pack_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_PANT_MARCA    0x50414E54u   /* 'PANT' */
/* La versión es una puerta EXACTA (`bpvm_pack_api_verify` compara con ==): sube
 * sólo si cambia la FIRMA de algo que ya existe. Añadir una función opcional NO
 * la sube — se resuelve por nombre, y los drivers ya grabados simplemente no la
 * publican. */
#define BPVM_PANT_API_VER  1u

/* ─── LOS FORMATOS DE PÍXEL (lo que devuelve `formato`) ───
 *
 *   RGB565     16 bits, en el orden de la CPU (little-endian). Un LTDC, un
 *              MIPI-DSI, un framebuffer: lo leen tal cual.
 *   RGB565_BE  16 bits con los dos bytes GIRADOS: lo que quiere un controlador
 *              MIPI-DCS por SPI (ST7789, ILI9341, GC9A01…), que recibe el byte
 *              alto primero. Lo gira el pack de LVGL con su rutina, antes del flush.
 *   RGB888     24 bits, 3 bytes por píxel. Para los controladores que por su
 *              interfaz NO admiten 16 bits —el caso típico, el ILI9488 por SPI,
 *              que sólo acepta 18 (RGB666) y se queda con los 6 bits altos de cada
 *              byte—. LVGL pinta directamente en 24 bits (es lo que hace su propio
 *              driver MIPI genérico): no hay conversión, cuesta búferes un 50 % más
 *              grandes.
 */
#define BPVM_PANT_RGB565     0
#define BPVM_PANT_RGB565_BE  1
#define BPVM_PANT_RGB888     2

/* La tabla ya resuelta: lo que el pack de LVGL tiene a mano después de buscar
 * 'PANT'. Las opcionales pueden ser NULL. */
typedef struct {
    int  (*init)   (void);
    int  (*size)   (int* w, int* h);
    void (*flush)  (int x1, int y1, int x2, int y2, const uint8_t* px);
    int  (*rotate) (int deg);
    int  (*touch)  (int* x, int* y);
    int  (*formato)(void);
} bpvm_pant_t;

/* Resuelve una tabla publicada bajo 'PANT' en `p`. Devuelve NULL si vale; si no,
 * el motivo (literal estático), con el NOMBRE de lo que falta cuando es eso.
 * `static inline` por lo mismo que `bpvm_pack_api_verify`: el que recoge la
 * tabla se lleva el verificador, y nadie gana un `.c` por esto. */
static inline const char* bpvm_pant_resolver(const bpvm_pack_api_t* api, bpvm_pant_t* p)
{
    if (api == 0 || p == 0) return "no hay tabla";
    {
        const char* mal = bpvm_pack_api_verify(api, BPVM_PANT_MARCA, BPVM_PANT_API_VER);
        if (mal) return mal;
    }
    p->init    = (int  (*)(void))                                  bpvm_pack_api_find(api, "init");
    p->size    = (int  (*)(int*, int*))                            bpvm_pack_api_find(api, "size");
    p->flush   = (void (*)(int, int, int, int, const uint8_t*))    bpvm_pack_api_find(api, "flush");
    p->rotate  = (int  (*)(int))                                   bpvm_pack_api_find(api, "rotate");
    p->touch   = (int  (*)(int*, int*))                            bpvm_pack_api_find(api, "touch");
    p->formato = (int  (*)(void))                                  bpvm_pack_api_find(api, "formato");
    if (p->init  == 0) return "al driver de pantalla le falta 'init'";
    if (p->size  == 0) return "al driver de pantalla le falta 'size'";
    if (p->flush == 0) return "al driver de pantalla le falta 'flush'";
    return 0;
}

/* Bytes por píxel de un formato (el pack de LVGL mide sus búferes con esto). */
static inline int bpvm_pant_bytes_por_pixel(int formato)
{
    return (formato == BPVM_PANT_RGB888) ? 3 : 2;
}

#ifdef __cplusplus
}
#endif
#endif /* BPVM_PANT_H */
