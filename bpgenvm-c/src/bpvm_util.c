/*
 * bpvm_util.c — DOS SERVICIOS PEQUEÑOS del núcleo, y por qué viven aparte.
 *
 * El canal de diagnóstico (#353) y la zona de rascar compartida (#338) los usan
 * el gestor de placa, las cinturas del FS y el propio wire. Ninguno de ellos
 * necesita la VM — pero mientras estos dos vivieron en bpvm.c, USARLOS obligaba
 * a enlazar la VM entera: intérprete, heap, builtins, GUI.
 *
 * Eso no era teórico: impedía escribir el test de #338. La batería de packs
 * enlaza sólo bpvm_pack.c a propósito (rápida y acotada), así que los tres
 * verbos que estrenaron la zona compartida se quedaban SIN PROBAR en host — y
 * un fallo del préstamo sólo habría salido en placa, que es el sitio caro.
 *
 * Ayer puse el canal de diagnóstico en bpvm.c justificándolo con la regla de
 * que un .c nuevo del núcleo son CINCO altas de build y olvidar una deja esa
 * familia sin enlazar. La regla es buena; la conclusión era la equivocada. Se
 * paga la matrícula una vez (Makefile + 3 CMakeLists; el STM32 recoge solos los
 * ficheros nuevos del linked folder al hacer cleanBuild) y ya está.
 *
 * Las declaraciones siguen en bpvm.h, que es donde se buscan.
 */

#include "bpvm.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ── #353: el canal de diagnóstico de la VM ──────────────────────────────────
 *
 * Vive aquí, en bpvm.c, A PROPÓSITO: un .c nuevo del núcleo hay que darlo de
 * alta en CINCO builds, y olvidar uno significa que esa familia no enlaza y no
 * te enteras hasta reconstruirla. bpvm.c ya está en los cinco.
 *
 * fflush OBLIGATORIO en el default: con stderr redirigido a fichero o tubería
 * (que es como corre el micro simulado bajo el IDE) el runtime lo vuelve
 * BUFFERIZADO, y si al proceso lo matan el aviso se queda dentro sin llegar a
 * nadie. Un guardián cuyo veredicto se pierde justo en el caso violento no
 * sirve para nada. */
static void diag_stderr(const char* linea) {
    fprintf(stderr, "%s\n", linea);   /* el '\n' lo pone EL SINK, no el llamante */
    fflush(stderr);
}
static bpvm_diag_fn g_diag = diag_stderr;

void bpvm_diag_set_sink(bpvm_diag_fn fn) { g_diag = fn ? fn : diag_stderr; }

/* ── #355: el canal URGENTE ──────────────────────────────────────────────────
 *
 * En la Pico `log_printf` sólo escribe en RAM; a flash llega en `log_flush()`,
 * que se llama en sitios contados. Consecuencia medida por Eduardo: cuando la
 * placa NO se cuelga el log se graba, y cuando SÍ se cuelga no se graba. O sea
 * que el instrumento nos falla exactamente en el caso que vinimos a investigar,
 * y encima de forma que parece "no pasó nada" en vez de "no llegué a contarlo".
 *
 * Esto no se arregla volcando cada línea: un borrado+programación de 4 KB por
 * aviso cuesta decenas de ms y desgasta la flash. Se arregla distinguiendo los
 * avisos que van SEGUIDOS DE UNA POSIBLE MUERTE —sin memoria, excepción sin
 * handler, bloque descarrilado— y volcando sólo esos. Son raros por definición:
 * si se repiten, el problema ya no es el coste del volcado. */
static void flush_nada(void) { }
static bpvm_diag_flush_fn g_diag_flush = flush_nada;

void bpvm_diag_set_flush(bpvm_diag_flush_fn fn) { g_diag_flush = fn ? fn : flush_nada; }

void bpvm_diag_urgente(const char* fmt, ...) {
    char linea[224];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(linea, sizeof linea, fmt, ap);
    va_end(ap);
    g_diag(linea);
    g_diag_flush();   /* que sobreviva a lo que venga detrás */
}

void bpvm_diag(const char* fmt, ...) {
    /* Buffer de PILA y acotado: esto se llama desde el loader y desde el fin de
     * RUN, en micros con la pila contada. 224 B es lo que ya usaba el guardián
     * y da de sobra para un nombre de módulo con su explicación. */
    char linea[224];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(linea, sizeof linea, fmt, ap);
    va_end(ap);
    g_diag(linea);
}

/* ── #338: la zona de rascar compartida. Contrato en bpvm.h ────────────────── */

/* Unión para que quede alineada como lo estaría un malloc: los usuarios no
 * guardan sólo bytes, también arrays de structs (bpvm_pack_info_t y compañía). */
static union {
    uint8_t  bytes[BPVM_SCRATCH_BYTES];
    uint64_t _u64;
    double   _f64;
    void*    _ptr;
} g_scratch;

/* Quién la tiene ahora mismo, o NULL. Un literal, no una copia: no hay que
 * reservar nada para saber a quién señalar. */
static const char* g_scratch_owner = NULL;

size_t bpvm_scratch_capacity(void) { return sizeof g_scratch.bytes; }

/* V7/H12, etapa 2b (5-oct) — mirar si está libre y quedársela, JUNTOS: con dos runs (y con `io`, que la pide para el
 * cable durante un RUN) dos podían verla libre a la vez y quedársela los dos. Con el cerrojo de la cola, que es
 * permanente, y que la VM REGISTRA aquí al crearla (bpvm_scratch_set_cerrojo, desde bpvm_cola_asegurar): este fichero
 * no depende de la VM —para eso vive aparte (arriba)—, y una herramienta que lo enlaza sin ella, con un solo hilo, no
 * necesita cerrojo. En el arranque aún no lo hay, y sólo hay un hilo. Nadie la pide con ese cerrojo ya cogido (el GC
 * y el planificador no la usan): no se anida. Ocupada, lo de siempre: NULL y el llamante falla con su nombre. */
static void zona_nada(void) { }
static bpvm_scratch_cerrojo_fn g_zona_cerrar = zona_nada;
static bpvm_scratch_cerrojo_fn g_zona_abrir  = zona_nada;

void bpvm_scratch_set_cerrojo(bpvm_scratch_cerrojo_fn cerrar, bpvm_scratch_cerrojo_fn abrir) {
    g_zona_cerrar = (cerrar && abrir) ? cerrar : zona_nada;
    g_zona_abrir  = (cerrar && abrir) ? abrir  : zona_nada;
}

static void zona_cerrar(void) { g_zona_cerrar(); }
static void zona_abrir(void)  { g_zona_abrir(); }

/* V7/H12, 2b — el MISMO cerrojo, para otros servicios sin VM que con dos runs comparten una tabla pequeña y no pueden
 * pedirle el suyo a la VM (la HAL: el anillo de la UART, las migas, el reloj). Secciones cortas y que no esperan; y
 * nunca con el cerrojo de la VM ya cogido (es el mismo, y no es recursivo). */
void bpvm_cerrojo_comun_cerrar(void) { g_zona_cerrar(); }
void bpvm_cerrojo_comun_abrir(void)  { g_zona_abrir(); }

static void* scratch_take(size_t n, const char* quien, int avisar) {
    const char* q = quien ? quien : "?";
    if (n > sizeof g_scratch.bytes) {
        /* NO se trunca: el llamante recibe NULL y decide. Truncar en silencio
         * una página de RMW sería escribir flash a medias. */
        bpvm_diag("[bpvm] scratch: '%s' pide %lu B y la zona son %lu",
                  q, (unsigned long) n, (unsigned long) sizeof g_scratch.bytes);
        return NULL;
    }
    zona_cerrar();
    const char* tenia = g_scratch_owner;
    if (!tenia) g_scratch_owner = q;
    zona_abrir();
    if (tenia) {
        if (avisar)
            bpvm_diag("[bpvm] scratch OCUPADA: la tiene '%s' y la pide '%s' "
                      "(dos operaciones a la vez que se creian exclusivas)",
                      tenia, q);
        return NULL;
    }
    return g_scratch.bytes;
}

void* bpvm_scratch_take(size_t n, const char* quien) { return scratch_take(n, quien, 1); }

/* V7/H12, 2b — la misma, sin el aviso si está ocupada: la usa quien va a esperar a que se suelte (un builtin con dos
 * runs, cuando la tiene el otro: builtins.c). Que no cabe sí se avisa: eso no se arregla esperando. */
void* bpvm_scratch_take_quieto(size_t n, const char* quien) { return scratch_take(n, quien, 0); }

void bpvm_scratch_give(const char* quien) {
    const char* q = quien ? quien : "?";
    zona_cerrar();
    const char* tenia = g_scratch_owner;
    g_scratch_owner = NULL;
    zona_abrir();
    if (!tenia) {
        bpvm_diag("[bpvm] scratch: '%s' la suelta sin haberla cogido", q);
        return;
    }
    if (quien && tenia != quien && strcmp(tenia, quien) != 0) {
        /* Soltar la zona de otro le deja el trabajo a medias sin que se entere.
         * Se avisa Y se suelta igual: quedársela colgada bloquearía a todos. */
        bpvm_diag("[bpvm] scratch: la suelta '%s' pero la tenia '%s'",
                  q, tenia);
    }
}

/* ── H13/#17: el parte del use-after-free ──────────────────────────────────
 * Tres números y una conclusión. La distinción que importa es si el handle
 * llegó con gen=0: eso NO es una referencia caducada, es un handle al que se le
 * cayeron los 32 bits altos por el camino (truncado a uint32 en algún sitio), y
 * sólo se manifiesta si ese slot se recicló alguna vez — de ahí que el mismo
 * programa vaya o no vaya según lo que se haya ejecutado antes.
 * Ya nos ha mordido tres veces: #302 (puente native→BP), heap_alloc_string y el
 * msg del RuntimeError. Que el mensaje lo diga solo. */
void bpvm_uaf_report(uint32_t idx, uint32_t gen_handle, uint32_t gen_slot,
                     uint32_t handle_next) {
    bpvm_diag_urgente("[bpvm] UAF: idx=%lu  gen del handle=%lu  gen del slot=%lu"
                      "  (slots en uso=%lu)",
                      (unsigned long) idx, (unsigned long) gen_handle,
                      (unsigned long) gen_slot, (unsigned long) handle_next);
    if (gen_handle == 0u) {
        bpvm_diag_urgente("[bpvm]   gen 0 con el slot en %lu: el handle perdio sus 32 bits"
                          " ALTOS. NO es una referencia caducada: es un TRUNCAMIENTO"
                          " (un bpref_t de 64b guardado en 32 en algun sitio).",
                          (unsigned long) gen_slot);
    } else {
        bpvm_diag_urgente("[bpvm]   las dos gen son != 0 y distintas: referencia CADUCADA"
                          " de verdad — el objeto se libero y alguien lo siguio usando.");
    }
}

/* V7/H12 (#660) — una referencia que NO PUEDE EXISTIR: sin TAG y con la palabra alta distinta de 0, o con TAG y un
 * índice fuera de la tabla. Lo normal es un programa que comparte un campo sin Mutex y lee la referencia mientras otro
 * run la escribe (la palabra alta de un valor y la baja de otro): el programa recibe «objeto eliminado» (bpvm_ref_dead).
 * Por el canal normal, no el urgente (un programa así puede dar muchas, y el urgente vuelca el log a la flash), y las
 * tres primeras: lo que se busca es el porqué, no la cuenta. */
void bpvm_ref_rota_report(uint32_t alta, uint32_t baja, uint32_t handle_next) {
    static unsigned dichas = 0;
    if (dichas >= 3u) return;
    dichas++;
    bpvm_diag("[bpvm] referencia ROTA: palabra alta 0x%08lx, baja 0x%08lx (slots en uso=%lu): a medio escribir por otro"
              " hilo (un campo compartido sin Mutex) o un indice que no existe; el programa recibe «objeto eliminado»",
              (unsigned long) alta, (unsigned long) baja, (unsigned long) handle_next);
}

/* V7/H12 (#660) — los caminos SIN ALINEAR de bpvm_read/write_u32/u16_be (bpvm_internal.h): byte a byte, como siempre.
 * Fuera de línea porque son los raros (los operandos del código); lo alineado, de una vez, va en línea. */
uint32_t bpvm_read_u32_be_bytes(const uint8_t* p) {
    return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) | ((uint32_t) p[2] << 8) | (uint32_t) p[3];
}
uint16_t bpvm_read_u16_be_bytes(const uint8_t* p) {
    return (uint16_t) (((uint32_t) p[0] << 8) | (uint32_t) p[1]);
}
void bpvm_write_u32_be_bytes(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t) (v >> 24); p[1] = (uint8_t) (v >> 16); p[2] = (uint8_t) (v >> 8); p[3] = (uint8_t) v;
}
void bpvm_write_u16_be_bytes(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t) (v >> 8); p[1] = (uint8_t) v;
}
