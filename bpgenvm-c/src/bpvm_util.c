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

void* bpvm_scratch_take(size_t n, const char* quien) {
    const char* q = quien ? quien : "?";
    if (n > sizeof g_scratch.bytes) {
        /* NO se trunca: el llamante recibe NULL y decide. Truncar en silencio
         * una página de RMW sería escribir flash a medias. */
        bpvm_diag("[bpvm] scratch: '%s' pide %lu B y la zona son %lu",
                  q, (unsigned long) n, (unsigned long) sizeof g_scratch.bytes);
        return NULL;
    }
    if (g_scratch_owner) {
        bpvm_diag("[bpvm] scratch OCUPADA: la tiene '%s' y la pide '%s' "
                  "(dos operaciones a la vez que se creian exclusivas)",
                  g_scratch_owner, q);
        return NULL;
    }
    g_scratch_owner = q;
    return g_scratch.bytes;
}

void bpvm_scratch_give(const char* quien) {
    const char* q = quien ? quien : "?";
    if (!g_scratch_owner) {
        bpvm_diag("[bpvm] scratch: '%s' la suelta sin haberla cogido", q);
        return;
    }
    if (quien && g_scratch_owner != quien && strcmp(g_scratch_owner, quien) != 0) {
        /* Soltar la zona de otro le deja el trabajo a medias sin que se entere.
         * Se avisa Y se suelta igual: quedársela colgada bloquearía a todos. */
        bpvm_diag("[bpvm] scratch: la suelta '%s' pero la tenia '%s'",
                  q, g_scratch_owner);
    }
    g_scratch_owner = NULL;
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
