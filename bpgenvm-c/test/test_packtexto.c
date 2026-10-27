/*
 * test_packtexto.c — [V8, #724] EL TEXTO QUE DEVUELVE UN PACK, por el puente que escribe el compilador.
 *
 * Publica un pack de mentira, 'PRUE', con dos funciones que dan el MISMO texto según el número (prb_texto, que
 * samples/PackTexto.bp declara `string`, y prb_bytes, `byte[]`), y corre el programa con el puente de AotMain
 * enlazado y el GC en cada reserva. Lo que tiene que salir lo dice el propio programa, escrito a mano: la receta
 * (`make test-packtexto`) cuenta las comprobaciones y no admite ningún FALLO.
 *
 * Los textos son los casos de la ficha: uno válido, uno de 600 bytes con una ñ partida por el corte de 512 de antes,
 * uno en Latin-1, el vacío, NULL, uno sin NUL en 1 MB (el tope del puente) y el más largo que cabe.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include "bpvm_aot_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void aot_PackTexto_register(struct bpvm* vm);

#define MARCA_PRUE 0x50525545u   /* 'PRUE' */

static char g_600[601];                           /* 511 «a», la ñ (C3 B1) en los bytes 511 y 512, 87 «b» y el NUL */
static char g_sin_nul[BPVM_AOT_PACK_TEXTO];       /* 1 MB de «x»: ningún NUL donde el puente mira */
static char g_maximo[BPVM_AOT_PACK_TEXTO];        /* 1 048 575 «y» y el NUL: lo más largo que cabe */

static const char* texto(int32_t n) {
    switch (n) {
    case 0: return "a\xC3\xB1o";
    case 1: return g_600;
    case 2: return "Mu\xF1oz";                    /* la ñ de Latin-1 */
    case 3: return "";
    case 4: return 0;
    case 5: return g_sin_nul;
    case 6: return g_maximo;
    default: return "?";
    }
}
static const char* prb_texto(int32_t n) { return texto(n); }
static const char* prb_bytes(int32_t n) { return texto(n); }

static const bpvm_pack_sym_t PUBLICS[] = {
    { "prb_texto", (bpvm_pack_fn_t) prb_texto },
    { "prb_bytes", (bpvm_pack_fn_t) prb_bytes },
};
static const bpvm_pack_api_t LA_TABLA = { MARCA_PRUE, 1u, 2u, PUBLICS };

int main(int argc, char** argv) {
    const char* mod_path = (argc > 1) ? argv[1] : "PackTexto.mod";
    size_t mem_size = 8u * 1024u * 1024u;   /* el caso 6 tiene vivas dos copias de 1 MB */
    uint8_t* mem;
    bpvm_t* vm;
    bpvm_status_t s;

    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);

    memset(g_600, 'a', 511);
    g_600[511] = (char) 0xC3; g_600[512] = (char) 0xB1;
    memset(g_600 + 513, 'b', 87);
    g_600[600] = 0;
    memset(g_sin_nul, 'x', sizeof g_sin_nul);
    memset(g_maximo, 'y', sizeof g_maximo - 1);
    g_maximo[sizeof g_maximo - 1] = 0;

    bpvm_bios_packs_reset();
    if (bpvm_bios_publica(MARCA_PRUE, &LA_TABLA) != 0) { fprintf(stderr, "el pack PRUE no se pudo publicar\n"); return 1; }

    mem = (uint8_t*) calloc(1, mem_size);
    if (!mem) { fprintf(stderr, "OOM\n"); return 1; }
    vm = bpvm_init(mem, mem_size, 0);
    if (!vm) { fprintf(stderr, "bpvm_init failed\n"); free(mem); return 1; }

    s = bpvm_load_mod(vm, mod_path);
    if (s != BPVM_OK) {
        fprintf(stderr, "load_mod %s: %s\n", mod_path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem); return (int) s;
    }

    aot_PackTexto_register(vm);    /* las native → el puente al pack */
    vm->gc_bump_threshold = 1;     /* GC en CADA reserva: el byte[] del puente tiene que estar a salvo */

    s = bpvm_run(vm);
    fprintf(stderr, "[status=%s]\n", bpvm_status_str(s));
    if (s != BPVM_OK)
        fprintf(stderr, "[runtime_error=%s]\n", bpvm_runtime_error(vm));

    bpvm_destroy(vm); free(mem);
    return (int) s;
}
