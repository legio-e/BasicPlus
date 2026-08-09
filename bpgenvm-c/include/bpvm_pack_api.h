/* bpvm_pack_api.h — lo que un pack PUBLICA: su tabla de símbolos.
 *
 * Es el espejo de `bpvm_bios.h`: aquélla es lo que el firmware PRESTA a un
 * pack, ésta lo que un pack DEVUELVE. Y las dos existen por lo mismo — el pack
 * se compila una vez, se congela y se graba, mientras el firmware y el IDE
 * siguen moviéndose.
 *
 * ─── ES EL MISMO MODELO QUE LOS MÓDULOS (idea de Eduardo, 9-ago) ─────────────
 *
 * Un `.mod` lleva una sección EXPORTS (lo que ofrece) y otra IMPORTS (lo que
 * necesita), y el loader las casa POR NOMBRE. Aquí igual, un nivel más arriba:
 *
 *      el pack        →  publics   (nombre → función)      ESTA tabla
 *      el `.mdn`      →  externals (nombres que necesita)
 *      el loader      →  los casa por nombre
 *
 * Lo que se gana frente a resolver por índice o por campo de `struct`:
 *
 *  · **El compilador no sabe nada de ningún pack.** Ni cabeceras, ni tipos, ni
 *    una lista de nombres por marca. Emite una llamada a un external con
 *    nombre, exactamente como para un `import` de módulo. LVGL entrará sin
 *    tocar el compilador.
 *  · **El desfase se caza POR NOMBRE.** Si el pack renombra o reordena algo, el
 *    loader no encuentra el símbolo y lo DICE — como un enlazador. Con índices,
 *    apuntaría a otra función y no protestaría nadie.
 *  · **No es un mecanismo nuevo.** Ya resolvíamos por nombre para módulos BP y
 *    para los thunks del AOT. Quien lea esto dentro de un año no tiene que
 *    aprender nada.
 *
 * La VERSIÓN sigue haciendo falta, pero como gate GRUESO: «¿hablamos el mismo
 * idioma?». La resolución por nombre es el fino: «¿está esta función?». Las dos,
 * no una u otra.
 *
 * ─── AQUÍ SÓLO CRUZAN TIPOS DE C ─────────────────────────────────────────────
 *
 * Ni un handle BP ni una cadena BP: el pack no puede alocar objetos BP —no
 * conoce el GC, y no debe—. Devuelve `const char*` UTF-8 y punteros crudos, y
 * convertirlos es trabajo de nuestro lado. Regalo de H2.1: las cadenas BP ya
 * son UTF-8, así que es copiar, no traducir.
 */
#ifndef BPVM_PACK_API_H
#define BPVM_PACK_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Un símbolo público del pack. El nombre es el que escribe `SQLite.bp` en su
 * `native function <nombre>(...)`, así que TIENE QUE SER UN IDENTIFICADOR
 * VÁLIDO DE BP y no una palabra reservada.
 *
 * Ya mordió una vez: el símbolo se llamaba `step` —el nombre natural— y `step`
 * es reservada en BP (el `for … step`). El compilador lo caza («se esperaba
 * nombre») así que no es un fallo mudo, pero es un viaje que se ahorra
 * mirándolo al añadir un símbolo. */
/* Puntero a función GENÉRICO, no `void*`. Convertir entre punteros a función
 * distintos y volver es conforme; meter una función en un `void*` es
 * formalmente indefinido (C99 §6.3.2.3). Quien recoja el símbolo lo casta a su
 * firma real antes de llamar — y esa firma la da la declaración BP. */
typedef void (*bpvm_pack_fn_t)(void);

typedef struct bpvm_pack_sym {
    const char*    nombre;
    bpvm_pack_fn_t fn;
} bpvm_pack_sym_t;

/* La cabecera de la tabla. Empieza por marca y versión a propósito: quien la
 * recoge tiene que poder comprobar que habla su idioma ANTES de leer nada más. */
typedef struct bpvm_pack_api {
    uint32_t                 marca;    /* 'SQLI', 'LVGL', 'PANT'…            */
    uint32_t                 version;  /* gate grueso                        */
    uint32_t                 n;        /* cuántos publics                    */
    const bpvm_pack_sym_t*   publics;
} bpvm_pack_api_t;

/*
 * Comprueba la tabla ANTES de usarla. Devuelve NULL si está bien; si no, una
 * frase con el motivo (literal estático).
 *
 * `static inline` en la cabecera a propósito: así el núcleo de la VM no gana un
 * .c por esto. La VM sólo guarda y devuelve el puntero bajo una marca; el que
 * lo recoge se lleva el verificador incluido.
 */
static inline const char* bpvm_pack_api_verify(const bpvm_pack_api_t* a,
                                               uint32_t marca_esperada,
                                               uint32_t version_esperada)
{
    uint32_t i;
    if (a == 0)                        return "no hay tabla";
    if (a->marca   != marca_esperada)  return "la marca no es la que se pidio";
    if (a->version != version_esperada) return "version de API distinta";
    if (a->n == 0 || a->publics == 0)  return "la tabla no publica nada";
    for (i = 0; i < a->n; i++) {
        /* Un hueco es un cuelgue esperando a ocurrir, y se ve AQUÍ sin ejecutar
         * nada del pack. Se comprueban los dos: un nombre sin función es tan
         * inútil como una función sin nombre — por nombre no se encontraría. */
        if (a->publics[i].nombre == 0) return "un simbolo sin nombre";
        if (a->publics[i].fn     == 0) return "un simbolo sin funcion";
    }
    return 0;
}

/*
 * Busca un símbolo por nombre. NULL = no está, y ése es el NULL que convierte
 * «el pack no trae esa función» en un mensaje con nombre en vez de un salto a
 * ninguna parte.
 *
 * Búsqueda lineal: son unas decenas de símbolos y se resuelve UNA vez, al
 * cargar. Ordenarlos para buscar en binario sería optimizar lo que no se mide,
 * y añadiría la obligación —fácil de olvidar— de mantener el orden.
 */
static inline bpvm_pack_fn_t bpvm_pack_api_find(const bpvm_pack_api_t* a, const char* nombre)
{
    uint32_t i;
    if (a == 0 || nombre == 0) return 0;
    for (i = 0; i < a->n; i++) {
        const char* p = a->publics[i].nombre;
        const char* q = nombre;
        while (*p && *p == *q) { p++; q++; }
        if (*p == 0 && *q == 0) return a->publics[i].fn;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
#endif /* BPVM_PACK_API_H */
