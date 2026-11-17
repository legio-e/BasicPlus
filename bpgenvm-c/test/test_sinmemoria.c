/*
 * test_sinmemoria.c — V8 #828 (17-nov): SIN MEMORIA AL CARGAR Y AL ENLAZAR, ¿DE CUÁL? Y #835: SIN FUGA.
 *
 * En la Pico 2, sin PSRAM, el bloque de la VM y el sistema (malloc) se reparten la misma RAM, y un programa de nueve
 * módulos (con Gui y Json) no enlazaba: «enlace tardio: sin memoria para construir las clases». Faltaba la del sistema
 * —las tablas del enlace las pide a malloc—, y el mensaje hacía pensar en darle más a la VM: justo al revés (con
 * `vmheap=288`, un bloque menor, enlaza). El cargador, igual, con el genérico del estado («memoria del buffer
 * insuficiente»). Ahora cada uno dice cuál: la del sistema, y qué hacer, o el sitio del bloque de la VM. Aquí, con
 * Json.mod (y Collections, Str y Core), cada caso forzado:
 *   0. la línea base, sin forzar nada: carga y corre;
 *   1. el enlace tardío sin memoria del sistema (la siguiente reserva de link.c falla: bpvm_alloc_prueba_fallar):
 *      «no hay memoria del sistema (malloc) para las tablas de las clases», con vmheap;
 *   2. el enlace tardío sin sitio en el bloque (el techo del heap, bajado a mano tras cargar): «no hay sitio en el
 *      bloque de la VM para construir las clases», sin malloc ni vmheap;
 *   3. la carga sin memoria del sistema (la siguiente reserva de loader.c falla): «'Json': no hay memoria del sistema
 *      (malloc) para sus tablas», con vmheap;
 *   4. la carga sin sitio en el bloque (el techo, bajado antes de cargar): «'Json' no cabe en el bloque de la VM».
 * Antes de #828, 1 y 2 decían lo mismo, y 3 y 4, el genérico: las cuatro comprobaciones de texto fallan.
 *
 * #835 (17-nov): y ninguno deja FUGA. Una carga que fallaba a medias no soltaba las tablas que el módulo ya había pedido
 * al sistema (el caso 4: 8 bloques, 5 KB, en una placa perdidos hasta reiniciar). En cada caso, los bloques vivos de la
 * VM (bpvm_alloc_live_blocks) son los mismos antes de crear la VM y después de destruirla; y
 *   5. la memoria del sistema se acaba en CADA punto de la carga: un presupuesto para lo que pide loader.c
 *      (bpvm_alloc_prueba_presupuesto), de 0 a lo que pide la carga entera, de 64 en 64 B; cada carga falla en una
 *      reserva distinta, de Json o de una dependencia, y ninguna deja fuga.
 * Sin el arreglo de #835, el 4 y el 5 fallan.
 *
 *   make test-sinmemoria
 */
#include "bpvm.h"
#include "bpvm_internal.h"   /* heap_top, next_free_address */
#include "bpvm_alloc.h"      /* bpvm_alloc_prueba_fallar / _presupuesto, bpvm_alloc_live_blocks */
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(c, m) do { if (c) printf("  ok  : %s\n", (m)); else { printf("  FAIL: %s\n", (m)); g_fail++; } } while (0)

enum { NADA, ENLACE_SISTEMA, ENLACE_BLOQUE, CARGA_SISTEMA, CARGA_BLOQUE };

typedef struct {
    bpvm_status_t carga, corre;
    long          gastado;         /* lo que loader.c pidió al cargar (con presupuesto) */
    int           fuga;            /* bloques de la VM que quedan vivos tras destruirla */
    char          error[3200];
} resultado_t;

/* `presupuesto`: lo que loader.c puede pedir en total al cargar (-1, sin límite). */
static resultado_t prueba(const char* mod, int caso, long presupuesto) {
    resultado_t r;
    memset(&r, 0, sizeof r);
    r.carga = r.corre = BPVM_ERR_RUNTIME;
    uint32_t vivos = bpvm_alloc_live_blocks(BPVM_ALLOC_VM);
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*) calloc(1, mem_size);
    bpvm_t* vm = mem ? bpvm_init(mem, mem_size, 0) : NULL;
    if (!vm) { snprintf(r.error, sizeof r.error, "sin memoria para la prueba"); free(mem); return r; }
    if (caso == CARGA_SISTEMA) bpvm_alloc_prueba_fallar("loader.c");
    if (caso == CARGA_BLOQUE)  vm->heap_top = vm->next_free_address + 64u;   /* el bloque, casi lleno */
    bpvm_alloc_prueba_presupuesto("loader.c", presupuesto >= 0 ? presupuesto : 0x7FFFFFFFL);
    r.carga = bpvm_load_mod(vm, mod);
    r.gastado = bpvm_alloc_prueba_gastado();
    bpvm_alloc_prueba_presupuesto(NULL, 0);
    bpvm_alloc_prueba_fallar(NULL);
    if (r.carga == BPVM_OK) {
        if (caso == ENLACE_SISTEMA) bpvm_alloc_prueba_fallar("link.c");
        if (caso == ENLACE_BLOQUE)  vm->heap_top = vm->next_free_address + 64u;
        r.corre = bpvm_run(vm);
        bpvm_alloc_prueba_fallar(NULL);
    }
    snprintf(r.error, sizeof r.error, "%s%s%s", bpvm_load_error(vm),
             (bpvm_load_error(vm)[0] && bpvm_link_error(vm)[0]) ? " | " : "", bpvm_link_error(vm));
    bpvm_destroy(vm);
    free(mem);
    r.fuga = (int) bpvm_alloc_live_blocks(BPVM_ALLOC_VM) - (int) vivos;
    return r;
}

static int dice(const resultado_t* r, const char* trozo) { return strstr(r->error, trozo) != NULL; }

int main(int argc, char** argv) {
    const char* mod = (argc > 1) ? argv[1] : "Json.mod";
    char m[3600];
    bpvm_fs_register_host();   /* el cargador busca las dependencias por la fachada */
    printf("--- #828 y #835: sin memoria al cargar y al enlazar, de cual, y sin fuga (%s) ---\n", mod);

    resultado_t b = prueba(mod, NADA, -1);
    snprintf(m, sizeof m, "0, sin forzar nada: carga y corre (%s / %s), sin fuga (%d); loader.c pide %ld B%s%s",
             bpvm_status_str(b.carga), bpvm_status_str(b.corre), b.fuga, b.gastado, b.error[0] ? "; dice: " : "", b.error);
    CHECK(b.carga == BPVM_OK && b.corre == BPVM_OK && b.fuga == 0 && b.gastado > 0, m);
    if (b.carga != BPVM_OK || b.corre != BPVM_OK) { printf("[status=FAIL: %d]\n", g_fail); return 1; }   /* sin base */

    resultado_t r = prueba(mod, ENLACE_SISTEMA, -1);
    snprintf(m, sizeof m, "1, el enlace tardio sin memoria del sistema: carga (%s), no enlaza (%s), fuga %d, y dice: \"%s\"",
             bpvm_status_str(r.carga), bpvm_status_str(r.corre), r.fuga, r.error);
    CHECK(r.carga == BPVM_OK && r.corre != BPVM_OK && r.fuga == 0
          && dice(&r, "no hay memoria del sistema (malloc) para las tablas de las clases") && dice(&r, "vmheap="), m);

    r = prueba(mod, ENLACE_BLOQUE, -1);
    snprintf(m, sizeof m, "2, el enlace tardio sin sitio en el bloque: carga (%s), no enlaza (%s), fuga %d, y dice: \"%s\"",
             bpvm_status_str(r.carga), bpvm_status_str(r.corre), r.fuga, r.error);
    CHECK(r.carga == BPVM_OK && r.corre != BPVM_OK && r.fuga == 0
          && dice(&r, "no hay sitio en el bloque de la VM para construir las clases")
          && !dice(&r, "malloc") && !dice(&r, "vmheap"), m);

    r = prueba(mod, CARGA_SISTEMA, -1);
    snprintf(m, sizeof m, "3, la carga sin memoria del sistema: no carga (%s), fuga %d, y dice: \"%s\"",
             bpvm_status_str(r.carga), r.fuga, r.error);
    CHECK(r.carga == BPVM_ERR_OOM && r.fuga == 0 && dice(&r, "no hay memoria del sistema (malloc) para sus tablas")
          && dice(&r, "vmheap="), m);

    r = prueba(mod, CARGA_BLOQUE, -1);
    snprintf(m, sizeof m, "4, la carga sin sitio en el bloque: no carga (%s), fuga %d (#835), y dice: \"%s\"",
             bpvm_status_str(r.carga), r.fuga, r.error);
    CHECK(r.carga == BPVM_ERR_OOM && r.fuga == 0 && dice(&r, "no cabe en el bloque de la VM")
          && !dice(&r, "malloc") && !dice(&r, "buffer insuficiente"), m);

    /* 5 (#835): la memoria del sistema se acaba en cada punto de la carga */
    int cargas = 0, fallan = 0, con_fuga = 0, sin_texto = 0;
    long primera_fuga = -1, bloques = 0;
    for (long p = 0; p < b.gastado; p += 64) {
        r = prueba(mod, NADA, p);
        cargas++;
        if (r.carga == BPVM_OK) continue;
        fallan++;
        if (!dice(&r, "no hay memoria del sistema (malloc)")) sin_texto++;
        if (r.fuga != 0) { con_fuga++; bloques += r.fuga; if (primera_fuga < 0) primera_fuga = p; }
    }
    snprintf(m, sizeof m, "5, sin memoria del sistema en cada punto de la carga (presupuesto de loader.c de 0 a %ld B, de 64 "
             "en 64): %d cargas, %d fallan, %d con fuga (%ld bloques; la primera, con %ld B), %d sin decir que es la del "
             "sistema", b.gastado, cargas, fallan, con_fuga, bloques, primera_fuga, sin_texto);
    /* no todas tienen que fallar: el último realloc de los nombres (encogerlos) puede no caber, y se sigue con el bloque
     * de antes (nombres_a_punteros) */
    CHECK(fallan > 10 && con_fuga == 0 && sin_texto == 0, m);

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
