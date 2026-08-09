/* bpsql_api.h — la IDENTIDAD del pack de SQLite. Nada más.
 *
 * Aquí ya no hay ninguna `struct` con la forma de la API. La tuvo, y se retiró
 * al adoptar el modelo de Eduardo (publics/externals casados por nombre, como
 * los módulos): la FORMA de la tabla es genérica y vive en `bpvm_pack_api.h`,
 * y lo único específico de SQLite que alguien necesita saber es con qué marca
 * se publica y qué versión habla.
 *
 * Eso es exactamente lo que se ganó por el camino: el que consume ya no tiene
 * que conocer la firma de cada función en C, porque la firma se la da su propia
 * declaración BP (`native function open(camino: string, soloLectura: boolean):
 * long`) y el símbolo lo encuentra por nombre.
 *
 * ─── LO QUE PUBLICA EL PACK (17 símbolos) ────────────────────────────────────
 *
 * No es una declaración, es documentación — la lista de verdad la construye
 * `sqlite_shim.c` y la comprueba el loader por nombre. Está aquí para que se
 * pueda leer sin abrir el shim:
 *
 *   conexión : open  exec  prepare  last_id  changes  close  errmsg
 *   consulta : fetch  col_count  col_name  col_type
 *              get_int  get_long  get_double  get_str  get_blob  release
 *
 * ⚠️ `fetch` y no `step`: el nombre es el que escribe `SQLite.bp` en su
 * `native function`, y `step` es palabra reservada en BP (`for … step`).
 *
 * ⚠️ VIDA de lo que devuelven `col_name`, `get_str` y `get_blob`: es memoria de
 * SQLite y vale hasta el siguiente `fetch` o el `release`. Hay que copiarla
 * ANTES; guardarse el puntero es un use-after-free que no falla el día que se
 * escribe.
 *
 * ─── Y EL GC, QUE ES LA PREGUNTA QUE SIEMPRE VUELVE ──────────────────────────
 *
 * Con esta API NO HAY PELIGRO DE GC, por CUATRO motivos que se sostienen a la
 * vez (razonamiento de Eduardo, 9-ago):
 *
 *  1. Lo que SQLite tiene entre manos vive en la arena que le prestamos
 *     (`SQLITE_CONFIG_HEAP`): el GC de BP ni la conoce ni la recorre.
 *  2. Los objetos BP están ENRAIZADOS por los locales de quien llama durante
 *     toda la llamada. Y `Db`/`Query` guardan un `long` —puntero crudo, no
 *     referencia—, así que el GC tampoco puede confundirlo con algo suyo.
 *  3. El GC sólo corre AL ASIGNAR, y de estos 17 símbolos sólo asignan los que
 *     devuelven cadena o blob, **una sola vez cada uno**. No hay ventana en la
 *     que el nativo sostenga un objeto BP sin raíz mientras asigna otro.
 *  4. El GC **no mueve objetos** (`heap.c`: "F2 v1: no compacta"), así que un
 *     puntero derivado dentro del nativo sigue valiendo mientras el objeto viva.
 *
 * Y esas cuatro se cumplen por cómo está DISEÑADA la librería —cada función
 * tiene principio y fin, no guarda estado entre llamadas y no encadena
 * asignaciones—, no por casualidad. El único fallo posible es una FUGA (nadie
 * suelta la consulta), no un uso-después-de-liberar.
 *
 * ⚠️ LO QUE HARÍA CAER EL PUNTO 3, y hay que recordarlo al AMPLIAR: un símbolo
 * que asigne DOS veces, o que sostenga un objeto BP mientras llama a algo que
 * asigna. Ahí sí entraría el paso 3 de #302 (raíces del GC del nativo
 * compilado), que sigue abierto.
 *
 * ⚠️ Y AL AÑADIR LOS PARÁMETROS (`?`), que hoy no existen: nunca dar a
 * `bind_text`/`bind_blob` un puntero a los bytes de una cadena BP con
 * `SQLITE_STATIC`. SQLite se lo queda hasta el `reset`, que puede sobrevivir al
 * local que enraizaba la cadena — y entonces el GC sí puede recogerla (moverla
 * no; ver punto 4). Siempre `SQLITE_TRANSIENT`, que copia.
 */
#ifndef BPSQL_API_H
#define BPSQL_API_H

#include "bpvm_pack_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 'SQLI' — con esta marca se publica y se busca. Cuatro bytes comparados con
 * `==`: no hay convención de nombres que alguien pueda escribir mal. */
#define BPSQL_MARCA    0x53514C49u

/* Gate GRUESO: «¿hablamos el mismo idioma?». Sube cuando cambia la SEMÁNTICA de
 * algún símbolo o desaparece uno. Añadir símbolos nuevos NO obliga a subirla —
 * el casado por nombre absorbe eso solo, que es la mitad de la gracia del
 * modelo. El gate FINO es la resolución por nombre: «¿está esta función?». */
#define BPSQL_VERSION  1u

/* Lo que devuelve `col_type`. Son los valores de SQLite, y coinciden con las
 * constantes TYPE_* de SQLite.bp — un sitio menos donde desalinearse. */
#define BPSQL_INTEGER  1
#define BPSQL_FLOAT    2
#define BPSQL_TEXT     3
#define BPSQL_BLOB     4
#define BPSQL_NULL     5

#ifdef __cplusplus
}
#endif
#endif /* BPSQL_API_H */
