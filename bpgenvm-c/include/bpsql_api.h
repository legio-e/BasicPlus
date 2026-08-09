/* bpsql_api.h — la tabla que el pack de SQLite OFRECE.
 *
 * Es el espejo de `bpvm_bios.h`. Aquella es lo que el firmware PRESTA a un
 * pack; ésta es lo que un pack DEVUELVE. Y las dos existen por el mismo motivo:
 * el pack se compila una vez, se congela y se graba, mientras el firmware y el
 * IDE siguen moviéndose. Lo único que puede cruzar esa frontera con seguridad
 * es una tabla de punteros con orden fijo y versión.
 *
 * ─── CÓMO SE ENCUENTRA ───
 *
 * El pack la publica al arrancar, en su propia entrada:
 *
 *     bios->publica(BPSQL_MARCA, &LA_TABLA);
 *
 * y quien la quiera la pide por su marca:
 *
 *     const bpsql_api_t* sql = bios->busca(BPSQL_MARCA);
 *     if (sql == 0) { … el usuario no grabó el pack de SQLite … }
 *
 * Ese NULL es lo que convierte «falta el pack» en un mensaje con nombre en vez
 * de en un salto a ninguna parte. Las dos ranuras son de la tabla BIOS v4 y son
 * LO ÚNICO que la VM sabe de todo esto: guarda un puntero bajo una marca y lo
 * devuelve. No conoce SQLite. Por eso añadir un pack no toca las 5 imágenes.
 *
 * ─── AQUÍ SÓLO CRUZAN TIPOS DE C ─────────────────────────────────────────────
 *
 * Ni un handle BP, ni una cadena BP, ni nada del heap de la VM. El pack no
 * puede alocar objetos BP —no conoce el GC, y no debe— así que devuelve lo que
 * SQLite le da: `const char*` UTF-8 y punteros crudos.
 *
 * Convertir eso en cadenas y arrays BP es trabajo de NUESTRO lado de la
 * frontera. Regalo de una decisión vieja: desde H2.1 las cadenas BP son UTF-8
 * en array de bytes y SQLite habla `const char*` UTF-8, así que la conversión
 * es copiar, no traducir.
 *
 * ⚠️ VIDA DE LO QUE DEVUELVEN `col_name`, `get_str` y `get_blob`: es de SQLite,
 * y vale **hasta el siguiente `fetch` o el `release`**. Hay que copiarlo ANTES.
 * Guardarse el puntero es un use-after-free que no falla el día que se escribe.
 *
 * ─── Y EL GC, QUE ES LA PREGUNTA QUE SIEMPRE VUELVE ──────────────────────────
 *
 * Con esta API NO HAY PELIGRO DE GC, y por CUATRO motivos que se sostienen a la
 * vez (razonamiento de Eduardo, 9-ago):
 *
 *  1. Lo que SQLite tiene entre manos vive en la arena que le prestamos
 *     (`SQLITE_CONFIG_HEAP`): el GC de BP ni la conoce ni la recorre.
 *  2. Los objetos BP están ENRAIZADOS por los locales de quien llama durante
 *     toda la llamada. Y `Db`/`Query` guardan un `long` —puntero crudo, no
 *     referencia—, así que el GC tampoco puede confundirlo con algo suyo.
 *  3. El GC sólo corre AL ASIGNAR, y de estas 17 funciones sólo asignan las que
 *     devuelven cadena o blob, **una sola vez cada una**. No hay ventana en la
 *     que el nativo sostenga un objeto BP sin raíz mientras asigna otro.
 *  4. El GC **no mueve objetos** (`heap.c`: "F2 v1: no compacta"), así que un
 *     puntero derivado dentro del nativo sigue valiendo mientras el objeto viva.
 *
 * Y esas cuatro se cumplen por cómo está DISEÑADA la librería —cada función
 * tiene principio y fin, no guarda estado entre llamadas y no encadena
 * asignaciones—, no por casualidad. El único fallo posible es una FUGA (nadie
 * suelta la consulta), no un uso-después-de-liberar.
 *
 * ⚠️ LO QUE HARÍA CAER EL PUNTO 3, y hay que recordarlo al AMPLIAR: una función
 * de puente que asigne DOS veces, o que sostenga un objeto BP mientras llama a
 * algo que asigna. Ahí sí entraría el paso 3 de #302 (raíces del GC del nativo
 * compilado), que está abierto.
 *
 * ⚠️ Y AL AÑADIR LOS PARÁMETROS (`?`), que hoy no existen: nunca dar a
 * `bind_text`/`bind_blob` un puntero a los bytes de una cadena BP con
 * `SQLITE_STATIC`. SQLite se lo queda hasta el `reset`, que puede sobrevivir al
 * local que enraizaba la cadena — y entonces el GC sí puede recogerla (moverla
 * no; ver punto 4). Siempre `SQLITE_TRANSIENT`, que copia.
 */
#ifndef BPSQL_API_H
#define BPSQL_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 'SQLI' — la marca con la que se publica y se busca. Cuatro bytes comparados
 * con `==`: no hay convención de nombres que alguien pueda escribir mal. */
#define BPSQL_MARCA    0x53514C49u

/* Sube al AÑADIR, QUITAR o REORDENAR campos. El que recoge la tabla compara y
 * se niega si no habla su versión. El pack se congela y el IDE no, así que ese
 * desfase VA a existir — mejor que grite. Mismo gate que el .mod (#284). */
#define BPSQL_VERSION  1u

/* ⚠️ LOS NOMBRES DE LOS CAMPOS SON IDENTIFICADORES DE BP, y eso es una
 * RESTRICCIÓN, no una casualidad.
 *
 * El enlace entre `SQLite.bp` y esta tabla es POR NOMBRE: una
 * `native function open(...)` sin cuerpo se resuelve al campo `open` de aquí.
 * Así que cada nombre tiene que ser un identificador válido de BP y NO una
 * palabra reservada.
 *
 * Ya mordió una vez: el campo se llamaba `step` —el nombre natural, y el de la
 * propia función de SQLite— y `step` es palabra reservada en BP (el `for … step`
 * de toda la vida). Por eso ahora se llama `fetch`, que además es el término
 * clásico de bases de datos para «trae la siguiente fila».
 *
 * Al añadir un campo: comprobarlo. El compilador lo caza —«se esperaba nombre»
 * en la declaración— así que no es un fallo silencioso, pero es un viaje de ida
 * y vuelta que se ahorra mirando la lista de keywords antes.
 */

/* Lo que devuelve `col_type`. Los valores son los de SQLite, y coinciden con
 * las constantes TYPE_* de SQLite.bp — un sitio menos donde desalinearse. */
#define BPSQL_INTEGER  1
#define BPSQL_FLOAT    2
#define BPSQL_TEXT     3
#define BPSQL_BLOB     4
#define BPSQL_NULL     5

typedef struct bpsql_api {
    uint32_t magic;                 /* BPSQL_MARCA                            */
    uint32_t version;               /* BPSQL_VERSION                          */

    /* ── LA CONEXIÓN ── */

    /* NULL = no se pudo abrir. Es un resultado NORMAL (tarjeta fuera, ruta
     * mala), no un error: en BP `connect` devuelve false y el programa decide. */
    void*       (*open)   (const char* camino, int solo_lectura);

    /* 0 = OK. Distinto de 0 = falló, y el motivo está en `errmsg`. */
    int         (*exec)   (void* db, const char* sql);

    /* NULL = el SQL no compila. El motivo, en `errmsg`. */
    void*       (*prepare)(void* db, const char* sql);

    int64_t     (*last_id)(void* db);
    int32_t     (*changes)(void* db);
    void        (*close)  (void* db);

    /* El texto del último fallo de ESA conexión. Nunca NULL: si no hay error,
     * devuelve una cadena vacía. Sin esto, un "falló" no dice nada y el
     * usuario se queda con un código en vez de con la frase que SQLite ya
     * había escrito. */
    const char* (*errmsg) (void* db);

    /* ── LA CONSULTA ── */

    /*  1 = hay fila
     *  0 = se acabaron
     * <0 = error de verdad (BD corrupta, tarjeta). En BP eso es excepción, y
     *      el texto sale de `errmsg` de su conexión. */
    int         (*fetch)     (void* st);

    int32_t     (*col_count) (void* st);
    const char* (*col_name)  (void* st, int32_t i);
    int32_t     (*col_type)  (void* st, int32_t i);   /* BPSQL_* */

    int32_t     (*get_int)   (void* st, int32_t i);
    int64_t     (*get_long)  (void* st, int32_t i);
    double      (*get_double)(void* st, int32_t i);
    const char* (*get_str)   (void* st, int32_t i);   /* UTF-8, nunca NULL */
    const void* (*get_blob)  (void* st, int32_t i, int32_t* bytes);

    void        (*release)   (void* st);              /* sqlite3_finalize */
} bpsql_api_t;

/*
 * Verifica la tabla ENTERA antes de usarla, igual que `bpvm_bios_verify` con la
 * de la BIOS: magic, versión y que ninguna ranura sea NULL. Un NULL en una
 * ranura es un cuelgue esperando a ocurrir, y es detectable AQUÍ, antes de
 * llamar a nada.
 *
 * Devuelve NULL si está completa; si no, el NOMBRE del primer campo que falta
 * (literal estático, nunca hay que liberarlo).
 *
 * Va `static inline` EN LA CABECERA a propósito: así el núcleo de la VM no gana
 * un fichero .c con forma de SQLite. La VM no sabe nada de esto y no debe —
 * sólo guarda y devuelve un puntero bajo una marca. Quien recoja la tabla se
 * lleva el verificador incluido.
 *
 * Se recorre por OFFSET y no con 17 `if` escritos a mano: esa forma se
 * desincroniza el día que alguien añade un campo y olvida su comprobación, y
 * entonces el hueco vuelve a ser mudo — que es lo que esto viene a evitar.
 */
static inline const char* bpsql_api_verify(const bpsql_api_t* a)
{
    static const struct { unsigned off; const char* nombre; } RANURAS[] = {
        { offsetof(bpsql_api_t, open), "open"       },
        { offsetof(bpsql_api_t, exec), "exec"       },
        { offsetof(bpsql_api_t, prepare), "prepare"    },
        { offsetof(bpsql_api_t, last_id), "last_id"    },
        { offsetof(bpsql_api_t, changes), "changes"    },
        { offsetof(bpsql_api_t, close), "close"      },
        { offsetof(bpsql_api_t, errmsg), "errmsg"     },
        { offsetof(bpsql_api_t, fetch), "fetch"      },
        { offsetof(bpsql_api_t, col_count), "col_count"  },
        { offsetof(bpsql_api_t, col_name), "col_name"   },
        { offsetof(bpsql_api_t, col_type), "col_type"   },
        { offsetof(bpsql_api_t, get_int), "get_int"    },
        { offsetof(bpsql_api_t, get_long), "get_long"   },
        { offsetof(bpsql_api_t, get_double), "get_double" },
        { offsetof(bpsql_api_t, get_str), "get_str"    },
        { offsetof(bpsql_api_t, get_blob), "get_blob"   },
        { offsetof(bpsql_api_t, release), "release"    },
    };
    const int N = (int)(sizeof RANURAS / sizeof RANURAS[0]);
    int i;

    if (a == 0)                      return "tabla";
    /* Magic y versión ANTES que las ranuras: si la tabla no es lo que creemos,
     * leer punteros de ella ya es leer basura. */
    if (a->magic   != BPSQL_MARCA)   return "magic";
    if (a->version != BPSQL_VERSION) return "version";

    for (i = 0; i < N; i++) {
        const void* const* p =
            (const void* const*) ((const char*) a + RANURAS[i].off);
        if (*p == 0) return RANURAS[i].nombre;
    }
    return 0;                        /* completa */
}

/* Para el log del arranque: «tabla de SQLite lista (17 ranuras)» dice más que
 * «lista», y si el número no es el esperado el desfase se ve de un vistazo. */
#define BPSQL_RANURAS 17

#ifdef __cplusplus
}
#endif
#endif /* BPSQL_API_H */
