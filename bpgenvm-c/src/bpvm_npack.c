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

    /* 4 — ¿llegó realojado? Un pack recién distribuido trae su tabla; lo que se
     *     GRABA ya no. Que el contador sea 0 ES la prueba de que pasó por el
     *     IDE — no hay un campo "ya_realojado" aparte que pueda mentir.
     *
     *     ⚠️ VA ANTES QUE EL SELLO, y el orden no es cosmético (V5/H8, 12-ago).
     *     Un pack sin realojar NO TIENE sello puesto, así que comprobarlo antes
     *     hacía que fallara por el peldaño equivocado: decía "realojado para
     *     OTRA direccion" cuando la verdad era "no se ha realojado nunca". Los
     *     dos mandan al IDE, así que el resultado coincidía y el error habría
     *     pasado inadvertido — pero el diagnóstico mentía, que es justo lo que
     *     esta escalera existe para evitar.
     *
     *     Y con el modelo de V5/H8 —el IDE reloca AL GRABAR— esto deja de ser
     *     un caso raro: un pack distribuido SIN realojar pasa a ser lo NORMAL.
     *     Juzgar su sello es juzgar una dirección que nadie le ha asignado. */
    if (h->reloc_count != 0)                      return BPVM_NPACK_E_SIN_RELOC;

    /* 5 — EL SELLO. El pack lleva punteros absolutos dentro; si está en otro
     *     sitio del que se realojó, apuntan a cualquier parte. Caso NORMAL:
     *     alguien cambió SQLite=<MB> y el bloque de RAM se movió. */
    if (h->linked_flash != aqui_flash || h->linked_ram != aqui_ram)
                                                  return BPVM_NPACK_E_SELLO;

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

/*
 * Busca un pack válido barriendo la zona. Ver la explicación en bpvm_npack.h.
 *
 * Mismo patrón que `bpvm_ancla_buscar`: avanzar de 4 en 4, descartar barato por
 * el magic, y sólo entonces gastar en la comprobación completa.
 */
bpvm_npack_hallazgo_t bpvm_npack_buscar(const void* base, uint32_t bytes,
                                        uint32_t aqui_ram, uint32_t sitio_ram,
                                        const char* bios_falta)
{
    bpvm_npack_hallazgo_t h;
    h.addr = 0; h.candidatos = 0; h.motivo = BPVM_NPACK_E_MAGIC;

    if (base == 0 || bytes < sizeof(bpvm_npack_hdr_t)) return h;

    const unsigned char* p = (const unsigned char*) base;
    uint32_t i = 0;
    /* Alinear el arranque: la cabecera lleva enteros de 32 bits y el que la
     * graba la deja a 4. Mirar las posiciones intermedias sería tiempo tirado. */
    while ((((uintptr_t) p + i) & 3u) != 0u) i++;

    for (; i + sizeof(bpvm_npack_hdr_t) <= bytes; i += 4) {
        const bpvm_npack_hdr_t* c = (const bpvm_npack_hdr_t*) (const void*) (p + i);
        if (c->magic != BPVM_NPACK_MAGIC) continue;   /* descarte barato */
        h.candidatos++;

        /* Dónde estaría el CÓDIGO si este candidato fuera bueno: la cabecera va
         * delante, así que el código empieza HDR_BYTES más allá. Ojo, ese es el
         * `aqui_flash` que espera la escalera — confundirlo con la base del pack
         * son 64 bytes de desfase (dicho también en el parámetro). */
        uint32_t aqui_flash = (uint32_t) (uintptr_t) (p + i) + BPVM_NPACK_HDR_BYTES;
        uint32_t queda      = bytes - i - BPVM_NPACK_HDR_BYTES;

        h.motivo = bpvm_npack_check(c, aqui_flash, aqui_ram, queda, sitio_ram,
                                    bios_falta);
        if (h.motivo == BPVM_NPACK_OK) {
            h.addr = (uint32_t) (uintptr_t) (p + i);
            return h;
        }
        /* Si no pasa, se sigue barriendo: puede haber otro más adelante. Pero el
         * motivo del último se conserva — un pack rechazado por el SELLO es una
         * noticia muy distinta de "aquí no hay nada". */
    }
    return h;
}
