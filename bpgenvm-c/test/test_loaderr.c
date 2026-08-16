/*
 * test_loaderr.c — #421: que un fallo de carga DIGA QUÉ HA PASADO.
 *
 * El 15-ago costó media mañana un `Core.mod` rancio en `/lib`: el IDE mostraba
 * `exit 1 (IO error)` y ya está. En esa función todos los errores de E/S son la
 * misma llamada fallando, así que «no existe», «mide cero», «está truncado» y
 * «la ruta venía vacía» salían idénticos — y ninguno decía la ruta, teniéndola
 * en la mano.
 *
 * Esto fija los cuatro mensajes. No comprueba que sean bonitos: comprueba que
 * son DISTINTOS entre sí y que llevan el dato que ahorra la búsqueda (la ruta).
 * Un mensaje que no distingue dos causas es el bug que esta ficha arregla.
 */
#include "bpvm.h"
#include "bpvm_entry.h"
#include "bpvm_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0;
static void ok(int c, const char* m) {
    printf(c ? "  ok  : %s\n" : "  FAIL: %s\n", m);
    if (!c) fallos++;
}

/* Deja en `out` el `fallo` que reporta cargar `path`. */
static void carga(const char* path, char* out, size_t cap) {
    static uint8_t mem[256 * 1024];
    memset(mem, 0, sizeof mem);
    bpvm_t* vm = bpvm_init(mem, sizeof mem, 0);
    bpvm_entry_t e;
    memset(&e, 0, sizeof e);
    bpvm_status_t s = bpvm_load_entry(vm, path, &e);
    snprintf(out, cap, "%s", (s == BPVM_OK) ? "(cargó)" : e.fallo);
    bpvm_destroy(vm);
}

int main(void) {
    bpvm_fs_register_host();

    char no_existe[192], vacio[192], roto[192], sin_ruta[192];

    carga("no_esta_por_ningun_lado.mod", no_existe, sizeof no_existe);
    ok(strstr(no_existe, "no encuentro") != NULL, "no existe: dice que no lo encuentra");
    ok(strstr(no_existe, "no_esta_por_ningun_lado.mod") != NULL,
       "no existe: dice QUÉ buscaba");
    ok(strstr(no_existe, "/lib") != NULL, "no existe: dice DÓNDE ha buscado");

    { FILE* f = fopen("t_vacio.mod", "wb"); fclose(f); }
    carga("t_vacio.mod", vacio, sizeof vacio);
    ok(strstr(vacio, "0 bytes") != NULL, "vacío: dice que mide cero");
    ok(strstr(vacio, "t_vacio.mod") != NULL, "vacío: dice la RUTA");

    /* Cabecera plausible pero contenido a medias — el caso del 15-ago. */
    { FILE* f = fopen("t_roto.mod", "wb");
      const unsigned char h[] = { 'B','P','M','6', 0,0,0,1, 0,0,0,9, 0,0,0,0 };
      fwrite(h, 1, sizeof h, f); fclose(f); }
    carga("t_roto.mod", roto, sizeof roto);
    ok(roto[0] != '\0', "truncado: dice algo");
    ok(strstr(roto, "t_roto.mod") != NULL, "truncado: dice la RUTA");

    carga("", sin_ruta, sizeof sin_ruta);
    ok(strstr(sin_ruta, "ruta") != NULL, "ruta vacía: lo dice");

    /* LO QUE DE VERDAD ARREGLA LA FICHA: que no sean el mismo mensaje. */
    ok(strcmp(no_existe, vacio) != 0 && strcmp(vacio, roto) != 0
       && strcmp(no_existe, roto) != 0 && strcmp(roto, sin_ruta) != 0,
       "los cuatro fallos dan mensajes DISTINTOS (antes: 'IO error' los cuatro)");

    remove("t_vacio.mod"); remove("t_roto.mod");
    printf("\n  no existe : %s\n  vacio     : %s\n  truncado  : %s\n  sin ruta  : %s\n",
           no_existe, vacio, roto, sin_ruta);
    printf("[status=%s]\n", fallos ? "FAIL" : "OK");
    return fallos ? 1 : 0;
}
