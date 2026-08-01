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
