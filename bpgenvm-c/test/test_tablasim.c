/*
 * test_tablasim.c — #639 (3-oct): LA TABLA DE SÍMBOLOS, CON POCA MEMORIA.
 *
 * En la C6 con la GUI había 16 KB de nombres en la tabla y el símbolo siguiente pedía 32 KB de un bloque —crecía siempre
 * al doble— a un `malloc` cuyo bloque mayor era de 7,9 KB. El fallo se tiraba (nadie miraba lo que devolvía el registro)
 * y el enlace acababa diciendo «presente pero no exporta… ¿versión vieja?». Ahora crece menos si el doble no cabe —×1,5,
 * ×1,25 y lo justo; Eduardo, 3-oct: «en vez de pasar de 8K a 16K, que pase a 10 o 12K»— y, si ni lo justo cabe, la carga
 * falla y lo dice.
 *
 * Aquí, Json.mod (con Collections, Str y Core: unos 7,5 KB de nombres) y un `malloc` que a link.c no le da más de N
 * bytes de un bloque (bpvm_alloc_prueba_tope):
 *   1. sin tope: la línea base —cuántos símbolos, cuántos bytes de nombres y la capacidad, una potencia de dos—;
 *   2. un tope entre los nombres y esa capacidad: el doble no cabe, y carga y enlaza IGUAL, con la tabla más pequeña;
 *   3. un tope por debajo de los nombres: la carga falla con BPVM_ERR_OOM y dice «sin memoria para la tabla de
 *      simbolos», no «presente pero no exporta».
 * Los topes salen de la línea base, no de un número escrito aquí: la stdlib crece. Sin el arreglo, la 2 falla (el doble
 * no cabe) y la 3 carga sin los símbolos.
 *
 *   make test-tablasim
 */
#include "bpvm.h"
#include "bpvm_internal.h"   /* la tabla: symbol_count, sym_pool_len, sym_pool_cap */
#include "bpvm_alloc.h"      /* bpvm_alloc_prueba_tope */
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(c, m) do { if (c) printf("  ok  : %s\n", (m)); else { printf("  FAIL: %s\n", (m)); g_fail++; } } while (0)

typedef struct {
    bpvm_status_t carga, corre;
    int           simbolos;
    unsigned      nombres, cap;
    char          error[600];
} resultado_t;

static resultado_t prueba(const char* mod, size_t tope) {
    resultado_t r;
    memset(&r, 0, sizeof r);
    r.carga = r.corre = BPVM_ERR_RUNTIME;
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    bpvm_t* vm = mem ? bpvm_init(mem, mem_size, 0) : NULL;
    if (!vm) { snprintf(r.error, sizeof r.error, "sin memoria para la prueba"); free(mem); return r; }
    bpvm_alloc_prueba_tope(tope ? "link.c" : NULL, tope);
    r.carga = bpvm_load_mod(vm, mod);
    if (r.carga == BPVM_OK) r.corre = bpvm_run(vm);       /* el enlace (y el tardío) también, con el tope puesto */
    bpvm_alloc_prueba_tope(NULL, 0);
    r.simbolos = vm->symbol_count;
    r.nombres  = vm->sym_pool_len;
    r.cap      = vm->sym_pool_cap;
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

    resultado_t b = prueba(mod, 0);
    snprintf(m, sizeof m, "1, sin tope: carga y enlaza (%s / %s); %d simbolos, %u B de nombres en %u B",
             bpvm_status_str(b.carga), bpvm_status_str(b.corre), b.simbolos, b.nombres, b.cap);
    CHECK(b.carga == BPVM_OK && b.corre == BPVM_OK && b.nombres > 2048 && b.cap >= b.nombres
          && (b.cap & (b.cap - 1)) == 0, m);
    if (g_fail) { printf("[status=FAIL: %d]\n", g_fail); return 1; }

    size_t tope = (b.nombres + b.cap) / 2;       /* el doble no cabe; lo que hace falta, sí */
    resultado_t r = prueba(mod, tope);
    snprintf(m, sizeof m, "2, tope %u B (el doble no cabe): carga y enlaza (%s / %s), %d simbolos y %u B de nombres "
             "en %u B —no %u— (esperado: los de la linea base, en menos del tope)%s%s",
             (unsigned) tope, bpvm_status_str(r.carga), bpvm_status_str(r.corre), r.simbolos, r.nombres, r.cap, b.cap,
             r.error[0] ? "; dice: " : "", r.error);
    CHECK(r.carga == BPVM_OK && r.corre == BPVM_OK && r.simbolos == b.simbolos && r.nombres == b.nombres
          && r.cap >= r.nombres && r.cap <= tope, m);

    tope = b.nombres - 200;                      /* ni lo justo cabe */
    r = prueba(mod, tope);
    snprintf(m, sizeof m, "3, tope %u B (menos que los nombres): la carga falla (%s) y lo dice: «%s»",
             (unsigned) tope, bpvm_status_str(r.carga), r.error);
    CHECK(r.carga == BPVM_ERR_OOM && strstr(r.error, "sin memoria para la tabla de simbolos") != NULL
          && strstr(r.error, "presente pero no exporta") == NULL, m);

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
