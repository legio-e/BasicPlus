/*
 * json_min.h — parser JSON minimal para el firmware bpvm-pico.
 *
 * Diseñado SOLO para el subset que usa el wire v1 de
 * docs/BPVM_WIRE_PROTOCOL.md:
 *
 *   - Objeto top-level plano: {"type":"...","id":N,"path":"/a","bulk":42}
 *   - Tipos de valor: string, número entero (i32/i64), boolean, null.
 *   - Objeto anidado de UN nivel (e.g. frame:{...}) — el valor se
 *     captura como string con el JSON crudo del objeto. El caller
 *     re-parsea si lo necesita.
 *   - Sin arrays anidados (no los usa el wire v1 en el lado del
 *     servidor Pico — sólo los REPLIES los necesitan, y los emite el
 *     server, no los parsea).
 *   - Sin floats (el wire v1 no tiene ningún campo float).
 *   - Sin escape exótico: solo \\, \", \n, \r, \t.
 *
 * Diseño:
 *   - In-place: el parser NO copia datos del buffer de entrada. Los
 *     valores devueltos son punteros + longitudes dentro del buffer
 *     original. Eso significa que el buffer NO puede modificarse
 *     mientras se accede a sus valores.
 *   - Sin allocs dinámicos. Array fijo de pares (key,value) en stack.
 *   - Sin reporte de errores detallado. Si la línea no es JSON
 *     válido, devolvemos -1 y el caller responde FATAL al peer.
 *
 * Coste estimado: ~150-200 líneas .c, ~3KB .text en Cortex-M33.
 */
#ifndef BPVM_JSON_MIN_H
#define BPVM_JSON_MIN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Máximo de pares (campo:valor) por mensaje. Los mensajes v1 más
 * grandes tienen ~10 campos; 16 deja margen. */
#define JSON_MAX_PAIRS  16

typedef enum {
    JSON_TYPE_STRING,    /* valor entre comillas; punteros sin las comillas */
    JSON_TYPE_NUMBER,    /* entero. Para acceso usa json_pair_long() */
    JSON_TYPE_BOOL,      /* true/false */
    JSON_TYPE_NULL,
    JSON_TYPE_OBJECT     /* valor "{...}" anidado — punteros incluyen { y } */
} json_value_type_t;

typedef struct {
    const char*       key;       /* puntero a la key dentro del buffer */
    size_t            key_len;
    const char*       value;     /* puntero al valor (raw, sin parsear) */
    size_t            value_len;
    json_value_type_t type;
} json_pair_t;

typedef struct {
    json_pair_t pairs[JSON_MAX_PAIRS];
    int         n_pairs;
} json_obj_t;

/* Parsea un objeto JSON top-level (debe empezar por '{' y terminar
 * por '}'). Devuelve 0 si OK, -1 si error de sintaxis o demasiados
 * pares para JSON_MAX_PAIRS.
 *
 * El buffer debe persistir mientras se usen los punteros del obj. */
int json_parse(const char* buf, size_t len, json_obj_t* out);

/* Busca un par por nombre de key. Devuelve NULL si no existe. */
const json_pair_t* json_find(const json_obj_t* obj, const char* key);

/* Helpers tipados — devuelven el valor o default si no existe o el
 * tipo no encaja. */
long        json_get_long  (const json_obj_t* obj, const char* key, long def);
int         json_get_bool  (const json_obj_t* obj, const char* key, int def);

/* Copia el valor string del par `key` a `dst` (null-terminated,
 * desescapado: \\n → 0x0a, \\\\ → \\, etc.). Devuelve la longitud
 * copiada (sin contar el null). Si el par no existe o no es string,
 * devuelve -1 y dst[0]=0.
 *
 * #456 — TRUNCAR YA NO ES SILENCIOSO: si el valor no cabe en dst_size
 * devuelve -2 (y dst lleva lo que cupo). Antes truncaba y devolvia la
 * longitud copiada, o sea que el llamador no tenia forma de distinguir
 * "cabia justo" de "no cabia": un PUT de un path largo escribia en OTRO
 * fichero y contestaba OK. Los llamadores comprueban < 0, asi que con -2
 * pasan a rechazar sin tocarlos.
 *
 * Para un path NO uses esta funcion: usa json_str_inplace, que no copia y
 * por tanto no tiene tope. */
int json_get_str(const json_obj_t* obj, const char* key,
                 char* dst, size_t dst_size);

/* #456 — Desescapa EN SITIO el valor string de `key`, dentro del mismo buffer
 * que se paso a json_parse, y devuelve un puntero a el terminado en NUL.
 *
 * NO COPIA, y por eso NO TIENE TOPE: lo que quepa en la linea que lo trajo,
 * cabe aqui. Es la forma correcta de leer un path — el tope de 64 que habia
 * antes no lo imponia ni el FS (255 por nombre en littlefs y en FatFs) ni el
 * protocolo (WIRE_V1_LINE_MAX), solo el buffer de la copia.
 *
 * REQUISITO: el buffer que se paso a json_parse tiene que ser ESCRIBIBLE.
 * Lo cumplen los cinco wires (todos parsean sobre su propio buffer de linea).
 *
 * CONSUME el par: desescapar encoge, asi que el NUL final cae dentro del hueco
 * que deja o sobre la comilla de cierre. Los demas pares no se tocan, pero la
 * linea deja de ser re-parseable y este par no se debe volver a leer.
 *
 * Devuelve NULL si el par no existe o no es string. */
char* json_str_inplace(json_obj_t* obj, const char* key);

/* Variante que NO desescapa: devuelve los punteros raw al string
 * dentro del buffer (sin comillas, con escapes literales). Útil para
 * casos donde el caller pasa el string sin transformarlo (e.g. spec
 * de PROMPT_REQUEST se reenvía tal cual). NULL si no existe. */
const char* json_get_str_raw(const json_obj_t* obj, const char* key,
                              size_t* out_len);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_JSON_MIN_H */
