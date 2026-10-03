/*
 * test_tablasim.c — #639 (3-oct): LA TABLA DE SÍMBOLOS, CON POCA MEMORIA.
 *
 * En la C6 con la GUI la tabla tenía 16 KB de nombres en UN bloque, y crecer pedía otro bloque entero, más grande: con
 * 18,6 KB libres pero el mayor hueco de 13,3 KB, no cabía (y antes de #639 el fallo se tiraba y el enlace acusaba a una
 * «versión vieja»). Ahora (Eduardo, 3-oct): los nombres van en TROZOS de 2 KB que no se mueven; el array de registros
 * crece menos si el doble no cabe (×1,5, ×1,25, lo justo); y lo que no cabe se APUNTA: la carga sigue y, si el enlace
 * busca uno que no está, dice la causa.
 *
 * Aquí, Json.mod (con Collections, Str y Core: unos 7,5 KB de nombres) con un `malloc` de mentira para link.c:
 *   1. sin límites: la línea base —los símbolos, los bytes de nombres, los trozos y lo que pide link.c al cargar—;
 *   2. TROCEADA: ningún bloque de más de 3 000 B, menos que los nombres juntos (bpvm_alloc_prueba_tope): carga y enlaza
 *      IGUAL —lo que en la C6 no cabía—;
 *   3. sin un solo hueco de 2 KB (tope 1 500): no cabe ningún trozo; la carga sigue, y el enlace, que los necesita, falla
 *      y dice la causa —«la tabla de simbolos se quedo sin memoria: N sin registrar»—, no «version vieja?»;
 *   4. se ACABA al cargar: lo que la línea base pidió al cargar, menos un byte (bpvm_alloc_prueba_presupuesto): lo último
 *      no entra; la carga sigue, y el programa corre si no usa lo que se quedó fuera —así corría GuiShot en la C6— o,
 *      si lo usa, el enlace dice la causa.
 * Los límites salen de la línea base, no de números escritos aquí: la stdlib crece. Sin los trozos, la 2 falla (pide
 * un bloque de más de 3 000 B); sin la causa, la 3; si la carga fallara (como en fd03af31), la 3 y la 4.
 *
 *   make test-tablasim
 */
#include "bpvm.h"
#include "bpvm_internal.h"   /* la tabla: symbol_count, sym_pool_len, sym_ntrozos, sym_perdidos */
#include "bpvm_alloc.h"      /* bpvm_alloc_prueba_tope / _presupuesto */
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(c, m) do { if (c) printf("  ok  : %s\n", (m)); else { printf("  FAIL: %s\n", (m)); g_fail++; } } while (0)

typedef struct {
    bpvm_status_t carga, corre;
    int           simbolos, perdidos, trozos;
    unsigned      nombres;
    long          gastado_carga;   /* lo que link.c pidió al cargar */
    char          error[600];
} resultado_t;

/* `tope`: ningún bloque de más (0 = sin tope), al cargar y al enlazar. `presupuesto`: lo que link.c puede pedir AL
 * CARGAR, en total (-1 = sin límite); al enlazar ya no cuenta, para que lo que se vea sea lo que se perdió al cargar. */
static resultado_t prueba(const char* mod, size_t tope, long presupuesto) {
    resultado_t r;
    memset(&r, 0, sizeof r);
    r.carga = r.corre = BPVM_ERR_RUNTIME;
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    bpvm_t* vm = mem ? bpvm_init(mem, mem_size, 0) : NULL;
    if (!vm) { snprintf(r.error, sizeof r.error, "sin memoria para la prueba"); free(mem); return r; }
    bpvm_alloc_prueba_tope(tope ? "link.c" : NULL, tope);
    bpvm_alloc_prueba_presupuesto("link.c", presupuesto >= 0 ? presupuesto : 0x7FFFFFFFL);
    r.carga = bpvm_load_mod(vm, mod);
    r.gastado_carga = bpvm_alloc_prueba_gastado();
    bpvm_alloc_prueba_presupuesto(NULL, 0);
    if (r.carga == BPVM_OK) r.corre = bpvm_run(vm);       /* el enlace (y el tardío), con el tope todavía puesto */
    bpvm_alloc_prueba_tope(NULL, 0);
    r.simbolos = vm->symbol_count;
    r.perdidos = vm->sym_perdidos;
    r.trozos   = vm->sym_ntrozos;
    r.nombres  = vm->sym_pool_len;
    snprintf(r.error, sizeof r.error, "%s%s%s", bpvm_load_error(vm),
             (bpvm_load_error(vm)[0] && bpvm_link_error(vm)[0]) ? " | " : "", bpvm_link_error(vm));
    bpvm_destroy(vm);
    free(mem);
    return r;
}

int main(int argc, char** argv) {
    const char* mod = (argc > 1) ? argv[1] : "Json.mod";
    char m[900];
    bpvm_fs_register_host();   /* el cargador busca las dependencias por la fachada */
    printf("--- #639: la tabla de simbolos con poca memoria (%s) ---\n", mod);

    resultado_t b = prueba(mod, 0, -1);
    snprintf(m, sizeof m, "1, sin limites: carga y enlaza (%s / %s); %d simbolos, %u B de nombres en %d trozos de "
             "2 KB; link.c pide %ld B al cargar", bpvm_status_str(b.carga), bpvm_status_str(b.corre), b.simbolos,
             b.nombres, b.trozos, b.gastado_carga);
    /* los trozos: los justos, o uno más por lo que sobra al final de cada uno (un nombre no se parte) */
    int justos = (int) ((b.nombres + 2047u) / 2048u);
    CHECK(b.carga == BPVM_OK && b.corre == BPVM_OK && b.nombres > 3000 && b.perdidos == 0
          && b.trozos >= justos && b.trozos <= justos + 1, m);
    if (b.carga != BPVM_OK || b.corre != BPVM_OK) { printf("[status=FAIL: %d]\n", g_fail); return 1; }   /* sin base */

    resultado_t r = prueba(mod, 3000, -1);
    snprintf(m, sizeof m, "2, troceada (ningun bloque de mas de 3000 B; los nombres son %u): carga y enlaza (%s / %s), "
             "%d simbolos y %u B de nombres, %d sin registrar (esperado: los de la linea base, 0)%s%s",
             b.nombres, bpvm_status_str(r.carga), bpvm_status_str(r.corre), r.simbolos, r.nombres, r.perdidos,
             r.error[0] ? "; dice: " : "", r.error);
    CHECK(r.carga == BPVM_OK && r.corre == BPVM_OK && r.simbolos == b.simbolos && r.nombres == b.nombres
          && r.perdidos == 0, m);

    r = prueba(mod, 1500, -1);
    snprintf(m, sizeof m, "3, sin un hueco de 2 KB (tope 1500 B): la carga sigue (%s, %d sin registrar), el enlace falla "
             "(%s) y dice la causa: «%s»", bpvm_status_str(r.carga), r.perdidos, bpvm_status_str(r.corre), r.error);
    CHECK(r.carga == BPVM_OK && r.perdidos > 0 && r.corre != BPVM_OK
          && strstr(r.error, "la tabla de simbolos se quedo sin memoria") != NULL
          && strstr(r.error, "version vieja") == NULL, m);

    r = prueba(mod, 0, b.gastado_carga - 1);
    snprintf(m, sizeof m, "4, se acaba al cargar (presupuesto %ld B, uno menos que la linea base): la carga sigue (%s, "
             "%d sin registrar) y el programa %s%s%s", b.gastado_carga - 1, bpvm_status_str(r.carga), r.perdidos,
             r.corre == BPVM_OK ? "corre (no usa lo que se quedo fuera: como GuiShot en la C6)"
                                : "no enlaza, y dice la causa: «",
             r.corre == BPVM_OK ? "" : r.error, r.corre == BPVM_OK ? "" : "»");
    CHECK(r.carga == BPVM_OK && r.perdidos > 0
          && (r.corre == BPVM_OK || strstr(r.error, "la tabla de simbolos se quedo sin memoria") != NULL), m);

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
