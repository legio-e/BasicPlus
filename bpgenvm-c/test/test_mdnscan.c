/*
 * test_mdnscan.c — que el `.mdn` se encuentre DENTRO del pack, y que el del FS
 * lo eclipse diciéndolo.
 *
 * ─── POR QUÉ ESTE TEST EXISTE ──────────────────────────────────────────────
 *
 * Lo que se añade en V5/H4 es que el escaneo mire también la zona de packs.
 * Eso sólo se ejerce de verdad en placa... y la placa hay que flashearla, y la
 * flashea Eduardo. Un cambio que sólo se puede probar así se convierte en
 * «pruébalo tú y ya me dirás», que es como se cuelan los fallos tontos.
 *
 * Pero la parte nueva NO tiene nada de específico del micro: es buscar en una
 * región de memoria y decidir quién gana. Y la región de packs es un puntero,
 * así que en el host se monta un buffer y ya está. Lo único que aquí no se
 * puede hacer es LLAMAR a los thunks — son ARM. Da igual: lo que se prueba es
 * que se encuentren, no que ejecuten (eso ya está probado en la Metro).
 *
 * El `.mdn` es el REAL (`build/sql/SQLite.mdn`, ARM Thumb-2, ABI 3) y el pack
 * lo construye el escritor REAL (`pack build` de Pack.jar). Un doble más
 * amable que el original no serviría: la gracia es que si el formato cambia,
 * esto se entera.
 *
 * El loader NO tiene gate de arquitectura en host —lo dice su propio comentario,
 * "(para tests)"— así que un .mdn de ARM se registra sin quejarse.
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_pack.h"
#include "bpvm_mdn_scan.h"
#include "aot_registry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0;

static void comprueba(int cond, const char* que) {
    if (cond) { printf("  ok   %s\n", que); }
    else      { printf("  FALLA %s\n", que); fallos++; }
}

/* ── lo que el escaneo nos cuenta ─────────────────────────────────────────── */

#define DICHOS_MAX 8
static char s_dichos[DICHOS_MAX][160];
static int  s_dichos_n = 0;

static void anota(void* user, const char* msg) {
    (void) user;
    if (s_dichos_n < DICHOS_MAX) {
        snprintf(s_dichos[s_dichos_n], sizeof s_dichos[0], "%s", msg);
        s_dichos_n++;
    }
    printf("      [dice] %s\n", msg);
}

static int dijo_que_contenga(const char* trozo) {
    for (int i = 0; i < s_dichos_n; i++)
        if (strstr(s_dichos[i], trozo)) return 1;
    return 0;
}

/* ── la cintura del FS, que aquí es de mentira a propósito ────────────────── */

static const uint8_t* s_fs_datos = NULL;
static uint32_t       s_fs_len   = 0;

static const uint8_t* fs_falso(void* user, const char* nombre, uint32_t* len) {
    (void) user;
    if (!s_fs_datos) return NULL;
    if (strcmp(nombre, "SQLite.mdn") != 0) return NULL;   /* sólo ése */
    *len = s_fs_len;
    return s_fs_datos;
}

/* ── utilidades ───────────────────────────────────────────────────────────── */

static uint8_t* leer_entero(const char* path, uint32_t* len_out) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    uint8_t* p = (uint8_t*) malloc((size_t) n);
    if (!p) { fclose(f); return NULL; }
    size_t got = fread(p, 1, (size_t) n, f);
    fclose(f);
    if (got != (size_t) n) { free(p); return NULL; }
    *len_out = (uint32_t) n;
    return p;
}

/* Una VM con UN módulo llamado como digamos. No se carga ningún .mod: al
 * escaneo sólo le hace falta la tabla de módulos, y montar un .mod de verdad
 * metería en la prueba un montón de cosas que no se están probando. */
static void vm_con_modulo(bpvm_t* vm, const char* nombre, int en_pack) {
    memset(vm, 0, sizeof *vm);
    vm->module_count = 1;
    snprintf(vm->modules[0].name, sizeof vm->modules[0].name, "%s", nombre);
    vm->modules[0].en_pack = (uint8_t) en_pack;   /* de donde salio el MODULO */
}

int main(int argc, char** argv) {
    const char* pack_path = (argc > 1) ? argv[1] : "build/sql/mdntest.pack";
    const char* mdn_path  = (argc > 2) ? argv[2] : "build/sql/SQLite.mdn";

    printf("== V5/H4: el .mdn dentro del pack ==\n");

    uint32_t pack_len = 0, mdn_len = 0;
    uint8_t* pack = leer_entero(pack_path, &pack_len);
    uint8_t* mdn  = leer_entero(mdn_path,  &mdn_len);
    if (!pack || !mdn) {
        printf("  SALTADO: falta %s o %s\n", pack_path, mdn_path);
        printf("  (los construye `make test-mdnscan`)\n");
        return 0;
    }
    printf("  pack %lu B · .mdn suelto %lu B\n",
           (unsigned long) pack_len, (unsigned long) mdn_len);

    bpvm_t vm;

    /* ── 1. Sin pack montado y sin FS: no encuentra nada, y eso NO es error ── */
    bpvm_pack_mount(NULL, 0);
    s_fs_datos = NULL; s_dichos_n = 0;
    bpvm_aot_clear();
    vm_con_modulo(&vm, "SQLite", 0);
    comprueba(bpvm_mdn_escanear(&vm, fs_falso, anota, NULL) == 0,
              "sin pack ni FS: 0 cargados");
    comprueba(s_dichos_n == 0, "sin pack ni FS: no dice nada (no hay noticia)");

    /* ── 2. SÓLO en el pack: lo encuentra ahí ─────────────────────────────── */
    bpvm_pack_mount(pack, pack_len);
    s_fs_datos = NULL; s_dichos_n = 0;
    bpvm_aot_clear();
    vm_con_modulo(&vm, "SQLite", 0);
    /* Devuelve 0 "cargados" A PROPOSITO: encontrarlo no es engancharlo. Esta
     * VM de prueba no tiene simbolos, asi que ni un thunk cuadra por nombre —
     * y el escaneo tiene que DECIRLO en vez de cantar victoria. Ese aviso lo
     * descubrio esta misma prueba: antes el mensaje decia "cargado" y punto. */
    comprueba(bpvm_mdn_escanear(&vm, fs_falso, anota, NULL) == 0,
              "sólo en el pack: lo ENCUENTRA (pero sin símbolos no engancha)");
    comprueba(dijo_que_contenga("se toma del pack") || dijo_que_contenga("del pack"),
              "dice que vino del pack");
    comprueba(dijo_que_contenga("NI UN thunk"),
              "AVISA de que no enganchó nada (si no, parece que funciona)");

    /* ── 3. EL CORAZON DE LA REGLA: el puente sigue a su modulo ───────────
     *
     * Con el .mdn en LOS DOS SITIOS, quien gana depende de donde salio el
     * MODULO. Es la correccion de Eduardo, y es de coherencia: un pack se
     * graba como un conjunto y un .mdn suelto no debe poder subvertirlo.
     *
     * Ojo con el falso PAR: los dos .mdn son el MISMO fichero, asi que no se
     * puede distinguir por el contenido. Lo que se comprueba es lo que el
     * escaneo DICE haber elegido — por eso el mensaje lleva la fuente. */
    s_fs_datos = mdn; s_fs_len = mdn_len;

    /* 3a. modulo del PACK -> manda el pack, aunque haya uno en el FS */
    s_dichos_n = 0; bpvm_aot_clear();
    vm_con_modulo(&vm, "SQLite", 1);
    bpvm_mdn_escanear(&vm, fs_falso, anota, NULL);
    comprueba(dijo_que_contenga("se toma del pack"),
              "modulo del PACK: su .mdn sale del PACK");
    comprueba(dijo_que_contenga("el del FS queda TAPADO"),
              "modulo del PACK: dice que el del FS queda tapado");

    /* 3b. modulo del FS -> manda el FS. Cada uno en lo suyo. */
    s_dichos_n = 0; bpvm_aot_clear();
    vm_con_modulo(&vm, "SQLite", 0);
    bpvm_mdn_escanear(&vm, fs_falso, anota, NULL);
    comprueba(dijo_que_contenga("se toma del FS"),
              "modulo del FS: su .mdn sale del FS");
    comprueba(dijo_que_contenga("el del pack queda TAPADO"),
              "modulo del FS: dice que el del pack queda tapado");

    /* 3c. modulo del PACK cuyo .mdn NO esta en el pack: cae al FS y lo DICE.
     *     Sin ese matiz, "primero el pack" se leeria como "solo el pack". */
    s_dichos_n = 0; bpvm_aot_clear();
    bpvm_pack_mount(NULL, 0);
    vm_con_modulo(&vm, "SQLite", 1);
    bpvm_mdn_escanear(&vm, fs_falso, anota, NULL);
    comprueba(dijo_que_contenga("pero su .mdn NO esta ahi"),
              "modulo del PACK sin .mdn dentro: cae al FS y lo explica");
    bpvm_pack_mount(pack, pack_len);

    /* ── 4. Un módulo que no tiene .mdn: ni ruido ni error ────────────────── */
    s_fs_datos = NULL; s_dichos_n = 0;
    bpvm_aot_clear();
    vm_con_modulo(&vm, "Str", 0);
    comprueba(bpvm_mdn_escanear(&vm, fs_falso, anota, NULL) == 0,
              "módulo sin .mdn: 0 cargados");
    comprueba(s_dichos_n == 0, "módulo sin .mdn: callado (es lo NORMAL)");

    /* ── 5. Sin cintura de FS (del_fs == NULL): el pack sigue valiendo ────── */
    s_dichos_n = 0;
    bpvm_aot_clear();
    vm_con_modulo(&vm, "SQLite", 0);
    bpvm_mdn_escanear(&vm, NULL, anota, NULL);
    comprueba(dijo_que_contenga("del pack"),
              "sin cintura de FS: el pack sigue siendo fuente");

    /* ── 6. Sin nadie a quien decírselo: no revienta ──────────────────────── */
    bpvm_aot_clear();
    vm_con_modulo(&vm, "SQLite", 0);
    bpvm_mdn_escanear(&vm, NULL, NULL, NULL);
    comprueba(1, "sin callback de mensajes: no revienta");

    free(pack); free(mdn);
    bpvm_pack_mount(NULL, 0);

    printf(fallos == 0 ? "\n[status=OK]\n" : "\n[status=FALLA] %d\n", fallos);
    return fallos == 0 ? 0 : 1;
}
