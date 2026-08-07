/* bpvm_npack.c — la escalera de validación del pack nativo.
 *
 * Portable y sin dependencias: aquí NO se lee flash ni se salta a ningún sitio.
 * Sólo se decide, con datos, si se puede — y si no, cuál es el peldaño.
 * Encontrar el pack y saltar son de la placa; esto es la parte que se puede
 * probar entera en el PC, que es donde conviene tenerla.
 */
#include "bpvm_npack.h"
#include "mdn_loader.h"      /* bpvm_mdn_host_arch / _float_abi: LA MISMA fuente
                              * que usa el gate del .mdn. Si la arch se dijera
                              * aquí de otra forma, un día dirían cosas distintas. */

/* Comparación de cadenas cortas sin arrastrar <string.h>: este fichero se
 * compila también donde libc es un lujo. */
static int igual(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

bpvm_npack_res_t bpvm_npack_check(const bpvm_npack_hdr_t* h,
                                  uint32_t aqui_flash, uint32_t aqui_ram,
                                  uint32_t sitio_flash, uint32_t sitio_ram,
                                  const char* bios_falta)
{
    /* 1 — ¿es un pack nativo? Lo más barato primero: si esto falla, todo lo
     *     demás sería interpretar basura. */
    if (h == 0 || h->magic != BPVM_NPACK_MAGIC)   return BPVM_NPACK_E_MAGIC;
    if (h->format != BPVM_NPACK_FORMAT)           return BPVM_NPACK_E_FORMAT;

    /* 2 — la ISA. Ejecutar Thumb en un RISC-V son instrucciones basura. */
    if (h->arch != bpvm_mdn_host_arch())          return BPVM_NPACK_E_ARCH;

    /* 3 — la convención de coma flotante. Y esta es la traicionera: NO da error
     *     de nada, da NÚMEROS MAL. `arch` sola no la distingue (medido: el
     *     toolchain guarda una libgcc por ABI). */
    if (!igual(h->float_abi, bpvm_mdn_host_float_abi()))
                                                  return BPVM_NPACK_E_ABI;

    /* 4 — EL SELLO. El pack lleva punteros absolutos dentro; si está en otro
     *     sitio del que se realojó, apuntan a cualquier parte. Caso NORMAL:
     *     alguien cambió SQLite=<MB> y el bloque de RAM se movió. */
    if (h->linked_flash != aqui_flash || h->linked_ram != aqui_ram)
                                                  return BPVM_NPACK_E_SELLO;

    /* 5 — ¿llegó realojado? Un pack recién distribuido trae su tabla; lo que se
     *     GRABA ya no. Que el contador sea 0 ES la prueba de que pasó por el
     *     IDE — no hay un campo "ya_realojado" aparte que pueda mentir. */
    if (h->reloc_count != 0)                      return BPVM_NPACK_E_SIN_RELOC;

    /* 6 — tamaños. Antes de copiar nada, que quepa; y que la entrada apunte
     *     DENTRO del código, no más allá. */
    if (h->flash_bytes > sitio_flash)             return BPVM_NPACK_E_TAMANO;
    if (h->data_bytes + h->bss_bytes > sitio_ram) return BPVM_NPACK_E_TAMANO;
    if (h->entry_off >= h->flash_bytes)           return BPVM_NPACK_E_TAMANO;
    /* Y la entrada alineada: en Thumb las instrucciones son de 2 bytes, una
     * entrada impar (ya sin el bit de modo) es un puntero corrupto. */
    if (h->entry_off & 1u)                        return BPVM_NPACK_E_TAMANO;

    /* 7 — la BIOS que le vamos a prestar. Se comprueba AQUÍ, lo último antes de
     *     saltar, porque un hueco se manifestaría dentro del pack: un cuelgue
     *     donde no hay depurador. */
    if (bios_falta != 0)                          return BPVM_NPACK_E_BIOS;

    return BPVM_NPACK_OK;
}

const char* bpvm_npack_res_str(bpvm_npack_res_t r)
{
    /* Con el REMEDIO cuando lo hay: un "no" que no dice cómo salir de él
     * obliga a abrir el código para entenderlo. */
    switch (r) {
        case BPVM_NPACK_OK:          return "listo";
        case BPVM_NPACK_E_MAGIC:     return "no es un pack nativo (magic)";
        case BPVM_NPACK_E_FORMAT:    return "formato de otra version — reconstruyelo";
        case BPVM_NPACK_E_ARCH:      return "compilado para OTRA arquitectura";
        case BPVM_NPACK_E_ABI:       return "otra convencion de coma flotante — daria numeros mal";
        case BPVM_NPACK_E_SELLO:     return "realojado para OTRA direccion — regrabalo desde el IDE";
        case BPVM_NPACK_E_SIN_RELOC: return "llego SIN realojar — tiene que pasar por el IDE";
        case BPVM_NPACK_E_TAMANO:    return "no cabe, o la entrada esta fuera del codigo";
        case BPVM_NPACK_E_BIOS:      return "la BIOS de esta placa tiene huecos";
        default:                     return "?";
    }
}

uint32_t bpvm_npack_entry_addr(const bpvm_npack_hdr_t* h, uint32_t aqui_flash)
{
    if (h == 0) return 0;
    uint32_t a = aqui_flash + h->entry_off;
    /* El bit 0 NO es parte de la dirección: le dice al núcleo que la función es
     * Thumb. Olvidarlo es un hard fault en la primera instrucción, y es el error
     * más fácil de cometer aquí — por eso vive en esta función y no repartido.
     * (Ya mordió en el lado PC: `nm` enmascara ese bit y decía "Thumb: no".) */
    if (h->flags & BPVM_NPACK_F_THUMB) a |= 1u;
    return a;
}
