/* mini.c — EL CONTROL DEL INSTRUMENTO: el pack nativo más pequeño que prueba
 * la cadena entera (V5, 7-ago; rehecho y VERSIONADO en V7/H6 — el original vivía
 * en `notas/` y se perdió al limpiar).
 *
 * Antes de grabar un motor de 600 KB en una placa nueva se graba ESTO. Si el
 * mini devuelve 0, la cadena funciona: compilar → pre-enlazar en base 0 →
 * realojar en el PC → empaquetar → grabar → el firmware lo encuentra, sube la
 * escalera, le copia la `.data`, le pone la `.bss` a cero y salta. Si un motor
 * falla después, el fallo es del motor. Sin el mini, un fallo de LVGL no se
 * distingue de un fallo del cargador — y el 7-ago el método fue exactamente
 * ése: mini v1 (la reja dice NO), mini v2 (dice SÍ), y sólo entonces SQLite.
 *
 * Devuelve 0 si TODO va, o el número del PASO que falló — y cada paso, antes,
 * lo cuenta por `log`, así que el log de la placa dice hasta dónde llegó:
 *
 *   1  no le dieron tabla
 *   2  la tabla no es una BIOS (magic)
 *   3  la BIOS es más VIEJA que la que este mini conoce (version)
 *   4  `log` a NULL (no habría podido contar nada de lo demás)
 *   5  un literal de `.rodata` no se realojó (la marca no se lee bien)
 *   6  las dos entradas de la tabla de funciones en `.data` (ABS32 a `.text`)
 *      no apuntan donde deben — el realojado de `.data` falla
 *   7  la `.bss` no llegó a cero (s_veces no vale 1 tras el primer paso)
 *   8  el reloj de la HAL (v5) no avanza
 *   9  `publica('MINI')` no aceptó su propia tabla, o `busca` no la devuelve
 *  10  `env_get` (v6, V7/H6.c G1) no cumple su contrato: una clave que no
 *      existe no dice «no está», una clave vacía no dice «argumentos», o un
 *      búfer sin sitio para el NUL da medio valor en vez de «no cabe»
 *
 * (Hasta G1 el paso 10 PEDÍA la arena. Ya no: desde G1 la arena tiene dueño
 * —se la queda el primer motor que la pide— y un control que la tomara se la
 * quitaría a SQLite si coincidieran en la misma placa.)
 *
 * Sólo depende de la tabla BIOS (`bpvm_bios.h`) y de `bpvm_pack_api.h`
 * (static inline): sin libc, sin libgcc — enlaza sin nada más. */
#include "bpvm_bios.h"
#include "bpvm_pack_api.h"
#include <stdint.h>

/* .rodata — un literal que sólo se lee bien si el realojado de .rodata va */
static const char s_marca[] = "mini: pack vivo (V7/H6.c, BIOS v6)";

/* .text — dos funciones a las que apunta una tabla en .data (dos ABS32) */
static int doble(int x)  { return x * 2; }
static int mas10(int x)  { return x + 10; }
/* `volatile` A PROPÓSITO: sin él, gcc ve una tabla estática que nadie escribe,
 * la pliega en llamadas directas y la .data desaparece — y con ella la prueba
 * (la primera versión salió con «datos 0 B» y el paso 6 no probaba nada). */
static int (*volatile s_tabla[2])(int) = { doble, mas10 };   /* .data: relocalizaciones a .text */

/* .bss — a cero por el cargador; la primera pasada lo deja en 1 */
static int s_veces;

/* lo que este mini PUBLICA: una tabla de dos símbolos bajo 'MINI' */
static void mini_hola(void) {}
static const bpvm_pack_sym_t s_syms[] = { { "hola", mini_hola }, { "adios", mini_hola } };
static const bpvm_pack_api_t s_api  = { 0x4D494E49u /* 'MINI' */, 1u, 2u, s_syms };

static void di(const bpvm_bios_t* b, const char* m) { if (b && b->log) b->log(m); }

int32_t bp_pack_init(const bpvm_bios_t* bios)
{
    if (bios == 0)                          return 1;
    if (bios->magic   != BPVM_BIOS_MAGIC)   return 2;
    if (bios->version <  BPVM_BIOS_VERSION) return 3;   /* crece por el final: >= vale */
    if (bios->log     == 0)                 return 4;

    di(bios, "mini: paso 4 ok (tabla, magic, version, log)");
    if (s_marca[0] != 'm' || s_marca[6] != 'p') return 5;
    di(bios, s_marca);                                  /* si esto sale legible, .rodata se realojó */

    if (s_tabla[0](21) != 42 || s_tabla[1](32) != 42)  return 6;
    di(bios, "mini: paso 6 ok (.data realojada: las dos ABS32 a .text)");

    s_veces++;
    if (s_veces != 1) return 7;
    di(bios, "mini: paso 7 ok (.bss a cero)");

    /* v5 — la HAL: el reloj tiene que estar y avanzar (o al menos no ir hacia atrás) */
    if (bios->now_ms == 0 || bios->sleep_ms == 0) return 8;
    { uint32_t a = bios->now_ms(); bios->sleep_ms(2); uint32_t b = bios->now_ms();
      if (b < a) return 8; }
    di(bios, "mini: paso 8 ok (la HAL v5: now_ms/sleep_ms)");

    /* el punto de encuentro, con la reja: la tabla dice 'MINI' y se publica como 'MINI' */
    if (bios->publica(0x4D494E49u, &s_api) != 0)       return 9;
    if (bios->busca(0x4D494E49u) != &s_api)            return 9;
    if (bpvm_pack_api_find((const bpvm_pack_api_t*) bios->busca(0x4D494E49u), "adios") == 0) return 9;
    di(bios, "mini: paso 9 ok (publica/busca 'MINI', 2 simbolos)");

    /* v6 — LA CONFIG DE PLACA (G1). El mini no sabe qué hay en el ENV de la
     * placa, así que prueba el CONTRATO (no está / argumentos / no cabe) y
     * enseña el valor de la clave que le dio RAM para cargar: `packRam` o
     * `SQLite` (en la Pico sin BD no hay ninguna, y no es fallo). */
    if (bios->env_get == 0) return 10;
    {
        char v[24];
        if (bios->env_get("", v, sizeof v) != BPVM_BIOS_ENV_ARGS)                      return 10;
        if (bios->env_get("miniNoExisteNunca", v, sizeof v) != BPVM_BIOS_ENV_NO_ESTA)  return 10;
        const char* clave = "packRam";
        int n = bios->env_get(clave, v, sizeof v);
        if (n < 0) { clave = "SQLite"; n = bios->env_get(clave, v, sizeof v); }
        if (n >= 0) {
            di(bios, clave[0] == 'p' ? "mini: env_get('packRam') vale:" : "mini: env_get('SQLite') vale:");
            di(bios, v);
            if (n > 0) {   /* sin sitio para el NUL: «no cabe» y el búfer vacío, no medio número */
                char w[24];
                w[0] = 'x';
                if (bios->env_get(clave, w, (size_t) n) != BPVM_BIOS_ENV_NO_CABE || w[0] != 0) return 10;
            }
        } else {
            di(bios, "mini: ni packRam ni SQLite en el ENV (bloque en la cola de la SRAM) - no es fallo");
        }
    }
    di(bios, "mini: paso 10 ok (env_get v6: no esta / argumentos / no cabe)");

    di(bios, "mini: TODO OK -> 0");
    return 0;
}
