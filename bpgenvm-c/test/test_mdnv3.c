/*
 * test_mdnv3.c — V7/H5 P2b.3: la marca DIRECTA del .mdn v3 llega al registro de natives.
 *
 * El escritor (MdnPack) pone la marca en el bit alto de `thunk_offset` de las natives DIRECTAS —sin puntos de
 * cesión: corren en la pila del worker, sin ejecutor—; el cargador (mdn_loader.c) la quita del offset y la pasa al
 * registro, que es donde la mira el intérprete. Esta prueba carga samples/NatDirecta.mod (sus nombres) y, sobre él,
 * blobs fabricados en memoria con la arquitectura del host y sin llegar a llamar a los thunks:
 *
 *   v3 con la marca en las cuatro directas → las cuatro, DIRECTA; las cinco demás, sin marca; y el thunk de cada
 *      una apunta a base + offset LIMPIO (la marca no se cuela en la dirección)
 *   el mismo blob como v2 (sin el bit)     → ninguna marcada: un .mdn de antes manda todas al ejecutor
 *   v4                                     → MDN_ERR_VERSION
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include "aot_registry.h"
#include "mdn_loader.h"
#include "mdn_format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0;
static void ok(int c, const char* m) { printf(c ? "  ok  : %s\n" : "  FAIL: %s\n", m); if (!c) fallos++; }

uint16_t bpvm_mdn_host_arch(void);
uint32_t bpvm_mdn_host_receta_crc(void);

static const char* DIRECTAS[]  = { "hoja", "llamaHoja", "fibR", "texto" };
static const char* EJECUTOR[]  = { "conBucle", "pequena", "llamaPequena", "conRaiz", "llamaBp" };
#define ND  4
#define NE  5
#define NS  (ND + NE)
#define CODIGO 64u

/* Un blob con los nueve símbolos: el i-ésimo en offset 4*i, con la marca si es directo y se pide. */
static uint8_t g_blob[24 + NS * 36 + CODIGO] __attribute__((aligned(8)));
static size_t fabricar(uint16_t version, int con_marca) {
    uint8_t* b = g_blob;
    uint32_t v;
    int i;
    memset(g_blob, 0, sizeof g_blob);
    b[0] = 'M'; b[1] = 'D'; b[2] = 'N'; b[3] = 0;
    b[4] = (uint8_t) version; b[5] = (uint8_t) (version >> 8);
    b[6] = (uint8_t) MDN_ABI_VERSION; b[7] = (uint8_t) (MDN_ABI_VERSION >> 8);
    v = CODIGO;                          memcpy(b + 8, &v, 4);
    v = NS;                              memcpy(b + 12, &v, 4);
    v = bpvm_mdn_host_arch();            memcpy(b + 16, &v, 4);
    v = bpvm_mdn_host_receta_crc();      memcpy(b + 20, &v, 4);
    for (i = 0; i < NS; i++) {
        uint8_t* s = b + 24 + i * 36;
        const char* f = i < ND ? DIRECTAS[i] : EJECUTOR[i - ND];
        uint32_t off = 4u * (uint32_t) i;
        snprintf((char*) s, 32, "NatDirecta.%s", f);
        if (con_marca && i < ND) off |= MDN_SIM_DIRECTA;
        memcpy(s + 32, &off, 4);
    }
    return sizeof g_blob;
}

static uint32_t addr_de(bpvm_t* vm, const char* qn) {
    for (int i = 0; i < vm->symbol_count; i++)
        if (strcmp(bpvm_symbol_name(vm, i), qn) == 0) return vm->symbols[i].abs_addr;
    return 0;
}

/* Cuántas de las nueve casan con lo esperado: registradas, con la marca que toca y el thunk en base + 4*i. */
static int comprobar(bpvm_t* vm, int directas_marcadas, char* m, size_t mlen) {
    int bien = 0, i;
    const uint8_t* base = g_blob + 24 + NS * 36;
    for (i = 0; i < NS; i++) {
        const char* f = i < ND ? DIRECTAS[i] : EJECUTOR[i - ND];
        char qn[48];
        uint32_t fl = 0xFFu;
        snprintf(qn, sizeof qn, "NatDirecta.%s", f);
        bpvm_aot_thunk_t t = bpvm_aot_lookup_flags(addr_de(vm, qn), &fl);
        int esperada = (directas_marcadas && i < ND) ? 1 : 0;
        uintptr_t dir = (uintptr_t) t & ~(uintptr_t) 1u;   /* sin el bit de modo (Thumb) si lo hubiera */
        if (t != NULL && ((fl & BPVM_AOT_DIRECTA) != 0) == esperada && dir == (uintptr_t) (base + 4u * (unsigned) i))
            bien++;
        else
            snprintf(m, mlen, "%s: thunk %p flags %u", qn, (void*) t, (unsigned) fl);
    }
    return bien;
}

int main(int argc, char** argv) {
    char m[200] = "";
    const char* mod_path = (argc > 1) ? argv[1] : "samples/NatDirecta.mod";
    size_t mem_size = 512 * 1024;
    uint8_t* mem;
    bpvm_t* vm;
    int n;
    bpvm_fs_register_host();
    printf("=== V7/H5 P2b.3: la marca DIRECTA del .mdn v3 llega al registro ===\n");
    if (bpvm_mdn_host_arch() == MDN_ARCH_NONE) {
        printf("  (arquitectura del host desconocida: el gate de arch no deja cargar)\n[status=OK]\n");
        return 0;
    }
    mem = (uint8_t*) calloc(1, mem_size);
    vm = mem ? bpvm_init(mem, mem_size, 0) : NULL;
    if (!vm || bpvm_load_mod(vm, mod_path) != BPVM_OK) {
        printf("  FAIL: no se pudo cargar %s\n\n[status=FAIL]\n", mod_path);
        return 1;
    }

    bpvm_aot_clear();
    ok(bpvm_load_mdn(vm, g_blob, fabricar(3, 1)) == MDN_OK, "v3 con la marca en las cuatro directas: se carga");
    n = comprobar(vm, 1, m, sizeof m);
    ok(n == NS, n == NS ? "  hoja, llamaHoja, fibR y texto, DIRECTAS; las otras cinco, sin marca; y cada thunk en base+offset LIMPIO"
                        : m);

    bpvm_aot_clear();
    ok(bpvm_load_mdn(vm, g_blob, fabricar(2, 0)) == MDN_OK, "el mismo blob como v2 (sin el bit, como los escribe MdnPack v2): se carga");
    n = comprobar(vm, 0, m, sizeof m);
    ok(n == NS, n == NS ? "  ninguna marcada: un .mdn de antes manda todas al ejecutor (correcto, sólo más lento)" : m);

    bpvm_aot_clear();
    ok(bpvm_load_mdn(vm, g_blob, fabricar(4, 1)) == MDN_ERR_VERSION, "v4: MDN_ERR_VERSION");

    bpvm_aot_clear();
    bpvm_destroy(vm);
    free(mem);
    printf("\n[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
