/*
 * bpvm_listdir.h — V5/H6 paso 3: LIST_DIR, el mismo en todas las familias.
 *
 * Nació en `pico/repl_v1.c` (V5/H2) y allí se quedó — de modo que el P4, que
 * usa el despachador del S3, no lo tenía. Y sin él el IDE cae al listado plano
 * (`LIST`), que recorre el FS ENTERO calculando el CRC de cada fichero: con una
 * tarjeta montada eso es leerse la tarjeta byte a byte. En el P4 se vio: el
 * árbol sale, pero castigando el bus.
 *
 * ─── LAS DOS PREGUNTAS, QUE SON DISTINTAS ──────────────────────────────────
 *
 *   · LIST     = «todo el FS interno, con CRC» — carga estructural del Run: el
 *                IDE se salta los PUT comparando ese CRC.
 *   · LIST_DIR = «los hijos de ESTE directorio» — lo que necesita MIRAR.
 *
 * Aquí no hay CRC ni recursión **a propósito**, no por ahorrar.
 *
 * ─── LA COSTURA: UN SUMIDERO DE TEXTO ──────────────────────────────────────
 *
 * Lo único que cambia entre familias es POR DÓNDE sale el texto: el Pico lo
 * escupe a `stdout` (su transporte es USB CDC) y el ESP32 lo trocea con
 * `wire_v1_send_bulk`. Las dos primitivas YA existían; no hubo que inventar la
 * frontera, sólo verla.
 *
 * Por eso el núcleo no devuelve una cadena montada: **la escribe a trozos**. Si
 * devolviera un buffer completo, 96 entradas serían ~9 KB de RAM en placas que
 * tienen 8 KB para todo el bulk — y forzaría a la familia que hoy puede
 * streamear a dejar de hacerlo.
 *
 * ─── Y POR QUÉ EN DOS TIEMPOS (fotografiar, luego emitir) ──────────────────
 *
 * No es un rodeo: el callback de la fachada corre DENTRO del cerrojo del FS.
 * Escribir al transporte desde ahí retendría el cerrojo todo lo que el host
 * tarde en leer, y cualquier thread BP que tocara un fichero se quedaría
 * esperando. `fs_list` ya lo hacía así por el mismo motivo.
 */
#ifndef BPVM_LISTDIR_H
#define BPVM_LISTDIR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Por dónde sale el texto. `n` es la longitud (el texto NO acaba en NUL). */
typedef void (*bpvm_txt_sink_t)(const char* txt, size_t n, void* user);

typedef enum {
    BPVM_LISTDIR_OK = 0,
    BPVM_LISTDIR_OCUPADO,   /* la zona de rascar está en uso  -> BUSY      */
    BPVM_LISTDIR_NO_LISTA   /* el camino no se puede listar   -> NOT_FOUND */
} bpvm_listdir_res_t;

/*
 * Emite `{"type":"LIST_DIR_REPLY","id":N,"entries":[…],"omitidas":K}` por el
 * sumidero. NO escribe el fin de línea: lo pone el llamante con su primitiva de
 * cierre, que es lo único que el transporte hace distinto.
 *
 * `path` NULL o vacío = la raíz.
 *
 * Con error NO emite nada — ni siquiera una lista vacía, que se leería como
 * «el directorio está vacío», que es mentira y de las caras. El llamante manda
 * su propio error por el wire.
 *
 * `omitidas` (si no es NULL) recibe cuántas entradas NO cupieron. El aviso viaja
 * también DENTRO del JSON: quien mira el listado es quien tiene que enterarse de
 * que no está entero.
 */
bpvm_listdir_res_t bpvm_listdir_emitir(const char* path, long id,
                                       bpvm_txt_sink_t sink, void* user,
                                       int* omitidas);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_LISTDIR_H */
