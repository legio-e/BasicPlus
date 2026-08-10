/*
 * bpvm_mdn_scan.c — el bucle que busca los `.mdn`, en UN sitio.
 * Ver bpvm_mdn_scan.h para el porqué, el orden de búsqueda y el estado.
 */
#include "bpvm_mdn_scan.h"
#include "bpvm_internal.h"
#include "bpvm_pack.h"
#include "mdn_loader.h"
#include "aot_registry.h"

#include <stdio.h>
#include <string.h>

/* El nombre de módulo cabe en 64 (bpvm_module_t.name), + ".mdn" + NUL. Holgado
 * a propósito: si algún día el nombre creciera, prefiero un buffer de sobra a
 * un -Wformat-truncation que alguien silencie. */
#define MDN_NOMBRE_MAX  80

static void decir(bpvm_mdn_decir_fn f, void* user, const char* msg) {
    if (f) f(user, msg);
}

int bpvm_mdn_escanear(struct bpvm* vm,
                      bpvm_mdn_del_fs_fn del_fs,
                      bpvm_mdn_decir_fn dicho,
                      void* user) {
    if (!vm) return 0;

    /* La zona de packs, si hay alguna montada. Se pide UNA vez: no cambia
     * durante el escaneo, y pedirla por módulo sería trabajo por nada. */
    uint32_t zona_len = 0;
    const uint8_t* zona = bpvm_pack_mounted(&zona_len);

    int cargados = 0;
    char msg[160];

    for (int i = 0; i < vm->module_count; i++) {
        const char* mod = vm->modules[i].name;
        if (!mod || !mod[0]) continue;

        char nombre[MDN_NOMBRE_MAX];
        snprintf(nombre, sizeof nombre, "%s.mdn", mod);

        /* ── Dónde está: las DOS fuentes, siempre ────────────────────────
         *
         * Se mira el pack aunque el FS ya lo tenga, y cuesta poco (recorrer un
         * índice). A cambio se puede AVISAR del eclipse, que es justo el caso
         * en que alguien pierde la tarde: subes un .mdn a mano para probar,
         * te olvidas de borrarlo, y meses después el del pack no se usa nunca
         * y nadie sabe por qué. */
        uint32_t pack_len = 0;
        const uint8_t* del_pack = zona
            ? bpvm_pack_find(zona, zona_len, "mdn", mod, &pack_len)
            : NULL;

        uint32_t fs_len = 0;
        const uint8_t* del_fichero = del_fs ? del_fs(user, nombre, &fs_len) : NULL;

        /* ── EL PUENTE SIGUE A SU MÓDULO ─────────────────────────────────
         *
         * Criterio de Eduardo: *«lo que está en un pack busca primero dentro
         * del pack y si no lo encuentra, busca fuera»*. Y el motivo es de
         * COHERENCIA, no de comodidad: un pack se graba como un conjunto —
         * motor, módulo y puente, una versión— y nada de fuera debería poder
         * subvertirlo. Si un `.mdn` suelto y olvidado en el FS ganara, algo
         * que siempre funcionó dejaría de funcionar sin que nadie tocara el
         * pack, y encima de forma difícil de ver.
         *
         * El reverso vale igual: si el módulo lo has subido TÚ al FS, su
         * puente se busca en el FS primero. Cada uno manda en lo suyo. */
        const uint8_t* datos;
        uint32_t        len;
        const char*     fuente;
        const char*     tapado = NULL;

        if (vm->modules[i].en_pack) {
            if (del_pack)         { datos = del_pack;    len = pack_len; fuente = "pack";
                                    if (del_fichero) tapado = "el del FS"; }
            else if (del_fichero) { datos = del_fichero; len = fs_len;   fuente = "FS (el modulo es del pack, pero su .mdn no esta ahi)"; }
            else                  continue;
        } else {
            if (del_fichero)      { datos = del_fichero; len = fs_len;   fuente = "FS";
                                    if (del_pack) tapado = "el del pack"; }
            else if (del_pack)    { datos = del_pack;    len = pack_len; fuente = "pack"; }
            else                  continue;   /* no hay .mdn: normal, y no es error */
        }

        /* Que haya DOS no es un error, pero decirlo ahorra tardes: es el caso
         * en que alguien sube un .mdn a mano para probar, se olvida de
         * borrarlo, y meses después mira el que no se está usando. */
        if (tapado) {
            snprintf(msg, sizeof msg, "AOT: %s se toma del %s; %s queda TAPADO",
                     nombre, fuente, tapado);
            decir(dicho, user, msg);
        }

        /* ── CUÁNTOS thunks entran de verdad ────────────────────────────────
         *
         * `bpvm_load_mdn` devuelve OK aunque registre CERO. Y tiene sentido
         * desde su punto de vista: el fichero estaba bien formado. Pero los
         * símbolos se enganchan POR NOMBRE contra el .mod cargado, así que un
         * .mdn que no corresponda a su .mod pasa todas las puertas —magic,
         * versión, ABI, arquitectura, layout— y no engancha ni uno.
         *
         * Sin esta cuenta, ese caso diría «cargado» y se seguiría interpretado.
         * Lo peor no es quedarse sin overlay: es que el mensaje diga que sí.
         * Luego eso se investiga como «va lento» en vez de como «el puente no
         * entró», que es donde está. Lo cazó la prueba de host al primer
         * intento, con una VM sin símbolos. */
        int antes = bpvm_aot_count();
        int rc = bpvm_load_mdn(vm, datos, (size_t) len);
        int nuevos = bpvm_aot_count() - antes;

        if (rc == MDN_OK && nuevos <= 0) {
            snprintf(msg, sizeof msg,
                     "AOT: %s del %s cargo pero NO engancho NI UN thunk — "
                     "¿no corresponde a este .mod? Se sigue interpretado",
                     nombre, fuente);
        } else if (rc == MDN_OK) {
            cargados++;
            snprintf(msg, sizeof msg, "AOT: %s cargado del %s (%lu B, %d thunks)",
                     nombre, fuente, (unsigned long) len, nuevos);
        } else {
            /* Un .mdn que no carga NO es fatal: se sigue interpretado. Pero se
             * dice con el rc, porque caer a interpretado en silencio es
             * exactamente la clase de cosa que luego se investiga como "está
             * lento" en vez de como "el overlay no entró". */
            snprintf(msg, sizeof msg,
                     "AOT: %s del %s NO carga (rc=%d) — se sigue interpretado",
                     nombre, fuente, rc);
        }
        decir(dicho, user, msg);
    }

    return cargados;
}
