/*
 * test_packhelp.c — V5/H4: los DOS helpers con los que un `native` llama a un pack.
 *
 * POR QUÉ ESTE FICHERO EXISTE
 * --------------------------
 * El emisor de AOT va a generar, para cada `native function` externa de un pack,
 * un thunk que hace exactamente dos cosas: convertir las cadenas BP a `char*` y
 * resolver el símbolo por nombre. Si cualquiera de las dos está mal, el fallo
 * aparece MUY lejos de aquí —dentro de SQLite, en la placa, con una BD abierta—
 * y no se parece a un fallo de conversión.
 *
 * Así que se prueban solos, en el PC, antes de que exista el emisor.
 *
 * LO QUE SE PRUEBA DE VERDAD
 *
 *  1. `string_to_cstr` NO TRUNCA EN SILENCIO. Es el punto entero: una cadena BP
 *     lleva longitud y no lleva NUL, y todo C quiere `const char*`. Truncar un
 *     camino de fichero abre OTRO fichero — y eso no se parece a un error, se
 *     parece a que la BD está vacía.
 *
 *  2. UN CARÁCTER NO ES UN BYTE. La cabecera de los helpers decía —hasta hoy—
 *     que un string BP era `TYPE_ARRAY_I32`, un codepoint de 4 bytes por
 *     carácter. Dejó de ser cierto en H2.1. Si me hubiera fiado del comentario,
 *     la conversión habría decodificado codepoints de 4 bytes sobre bytes UTF-8:
 *     no da error, da basura. Por eso el caso de "año" está aquí, con su
 *     comprobación de que la longitud en BYTES es 4 y no 3.
 *
 *  3. `pack_sym` devuelve NULL POR CADA MOTIVO DISTINTO por el que puede fallar,
 *     y el thunk necesita ese NULL para convertirlo en una excepción con nombre.
 *     Sin él, un pack que falta sería un salto a ninguna parte.
 */

#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"

#include <stdio.h>
#include <string.h>

static int fallos = 0;
static void ok(const char* que, int cond) {
    if (cond) printf("ok    %s\n", que);
    else    { printf("FALLO %s\n", que); fallos++; }
}

/* ── un pack de mentira, con la pinta EXACTA del de verdad ─────────────────── */
#define MARCA_FALSA   0x50525542u    /* 'PRUB' */
static int llamada_hecha = 0;
static int32_t falso_suma(int32_t a, int32_t b) { llamada_hecha = 1; return a + b; }
static void    falso_nada(void) { }

static const bpvm_pack_sym_t PUBLICS[] = {
    { "prb_suma", (bpvm_pack_fn_t) falso_suma },
    { "prb_nada", (bpvm_pack_fn_t) falso_nada },
};
static const bpvm_pack_api_t LA_TABLA = { MARCA_FALSA, 1u, 2u, PUBLICS };

/* ─────────────────────────────────────────────────────────────────────────── */
static uint8_t mem[256 * 1024];

int main(void)
{
    const aot_helpers_v2_t* H = &bpvm_aot_helpers_v2;
    bpvm_t* vm = bpvm_init(mem, sizeof mem, sizeof(mem) / 2);
    char buf[64];
    int32_t n;

    if (!vm) { printf("FALLO: bpvm_init\n"); return 1; }
    /* Recién inicializada la VM el heap está VACÍO (heap_start == stack_base):
     * quien le da sitio es la reserva de arena del loader al cargar un módulo.
     * Aquí no cargamos ninguno. Sin esto, `alloc_string` devuelve 0 y TODAS las
     * pruebas de cadena fallan por el banco, no por el helper. */
    if (!bpvm_arena_reserve(vm, 256, 4)) { printf("FALLO: arena\n"); return 1; }
    /* Y el GC fuera: estas cadenas no cuelgan de ninguna raíz —no hay programa
     * BP que las sostenga—, así que una pasada de GC se las llevaría por buenos
     * motivos y el fallo parecería del helper. */
    bpvm_set_gc_enabled(vm, 0);

    printf("--- V5/H4: los dos helpers del puente a un pack ---\n\n");

    printf("  -- string_to_cstr --\n");
    {
        uint32_t s = bpvm_heap_alloc_string(vm, "/sd/SmartMini.db", 16);
        memset(buf, '#', sizeof buf);
        n = H->string_to_cstr(vm, s, buf, (int32_t) sizeof buf);
        ok("devuelve la longitud en bytes", n == 16);
        ok("el contenido es el que era",    strcmp(buf, "/sd/SmartMini.db") == 0);
        ok("y queda terminada en NUL",      buf[16] == 0);

        /* Justo-justo: 16 bytes + el NUL = 17. Con 17 cabe; con 16 NO. */
        memset(buf, '#', sizeof buf);
        ok("cap == len+1 -> cabe justo",  H->string_to_cstr(vm, s, buf, 17) == 16);
        memset(buf, '#', sizeof buf);
        ok("cap == len   -> NO cabe (falta el NUL)", H->string_to_cstr(vm, s, buf, 16) == -1);
        ok("y NO deja media cadena: buffer vacio", buf[0] == 0);
        ok("cap = 1  -> no cabe",  H->string_to_cstr(vm, s, buf, 1) == -1);
        ok("cap = 0  -> se niega", H->string_to_cstr(vm, s, buf, 0) == -1);
        ok("dst NULL -> se niega", H->string_to_cstr(vm, s, 0, 64)  == -1);
    }
    {
        uint32_t vacia = bpvm_heap_alloc_string(vm, "", 0);
        memset(buf, '#', sizeof buf);
        ok("cadena vacia -> 0 y \"\"", H->string_to_cstr(vm, vacia, buf, 8) == 0 && buf[0] == 0);
        memset(buf, '#', sizeof buf);
        ok("ref 0 (null) -> 0 y \"\", no revienta",
           H->string_to_cstr(vm, 0, buf, 8) == 0 && buf[0] == 0);
    }
    {
        /* EL CASO QUE DESMIENTE EL COMENTARIO RANCIO: "año" son 3 caracteres
         * pero 4 BYTES (la ñ ocupa dos). Lo que cruza a C son los bytes. */
        uint32_t s = bpvm_heap_alloc_string(vm, "a\xC3\xB1o", 4);
        memset(buf, '#', sizeof buf);
        n = H->string_to_cstr(vm, s, buf, (int32_t) sizeof buf);
        ok("\"ano~\": devuelve 4 BYTES, no 3 caracteres", n == 4);
        ok("los bytes UTF-8 salen intactos", memcmp(buf, "a\xC3\xB1o", 5) == 0);
        ok("...y string_length sigue diciendo 3 (esa cuenta caracteres)",
           H->string_length(vm, s) == 3);
        /* Y el limite se mide en BYTES: con 4 no cabe aunque "quepan" 3 letras. */
        ok("el hueco se mide en bytes: cap=4 no cabe", H->string_to_cstr(vm, s, buf, 4) == -1);
    }

    printf("\n  -- pack_sym --\n");
    bpvm_bios_packs_reset();
    ok("sin ningun pack grabado -> NULL",
       H->pack_sym(MARCA_FALSA, 1u, "prb_suma") == 0);

    ok("el pack publica su tabla", bpvm_bios_publica(MARCA_FALSA, &LA_TABLA) == 0);
    {
        int32_t (*suma)(int32_t, int32_t) =
            (int32_t (*)(int32_t, int32_t)) H->pack_sym(MARCA_FALSA, 1u, "prb_suma");
        ok("resuelve por nombre", suma != 0);
        if (suma) {
            ok("y el puntero es LA funcion (2+3=5)", suma(2, 3) == 5 && llamada_hecha);
        }
    }
    ok("otra MARCA        -> NULL (ese pack no esta)",
       H->pack_sym(0x4C56474Cu, 1u, "prb_suma") == 0);
    ok("otra VERSION      -> NULL (gate grueso)",
       H->pack_sym(MARCA_FALSA, 99u, "prb_suma") == 0);
    ok("nombre que no esta-> NULL (gate fino)",
       H->pack_sym(MARCA_FALSA, 1u, "prb_step") == 0);
    ok("prefijo de otro   -> NULL, no coincidencia parcial",
       H->pack_sym(MARCA_FALSA, 1u, "prb_sum") == 0);
    ok("nombre NULL       -> NULL, no revienta",
       H->pack_sym(MARCA_FALSA, 1u, 0) == 0);

    printf("\n==========================================\n");
    printf(fallos == 0 ? "  [status=OK]\n" : "  [status=FAIL] %d\n", fallos);
    return fallos == 0 ? 0 : 1;
}
