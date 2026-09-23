/* bpvm_npack.c — la escalera de validación del pack nativo, y (V7/H6.c G1) el
 * PLAN de carga cuando la zona lleva varios motores.
 *
 * Portable y sin dependencias: aquí NO se lee flash ni se salta a ningún sitio.
 * Sólo se decide, con datos, si se puede — y si no, cuál es el peldaño.
 * Encontrar la zona y saltar son de la placa; esto es la parte que se puede
 * probar entera en el PC, que es donde conviene tenerla.
 */
#include "bpvm_npack.h"
#include "bpvm_pack.h"       /* la cadena: iterar packs y entradas, SIN copiar */
#include "mdn_loader.h"      /* bpvm_mdn_host_arch / _float_abi: LA MISMA fuente
                              * que usa el gate del .mdn. Si la arch se dijera
                              * aquí de otra forma, un día dirían cosas distintas. */

/* Comparación de cadenas cortas sin arrastrar <string.h>: este fichero se
 * compila también donde libc es un lujo. */
static int igual(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
static void copia(char* dst, const char* src, int cap) {
    int i = 0;
    while (i < cap - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

bpvm_npack_res_t bpvm_npack_escalera(const bpvm_npack_hdr_t* h,
                                     uint32_t aqui_flash, uint32_t sitio_flash,
                                     uint32_t ram_lo, uint32_t ram_hi,
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
     *     sitio del que se realojó, apuntan a cualquier parte. En flash tiene
     *     que ser EXACTAMENTE aquí. En RAM, desde G1, DENTRO del bloque: el
     *     trozo se lo dio la placa al grabarlo, y el segundo motor va detrás del
     *     primero. Caso NORMAL de fallo: alguien cambió SQLite=<MB> o
     *     packRam=<KB> y el bloque se movió. */
    if (h->linked_flash != aqui_flash)            return BPVM_NPACK_E_SELLO;
    /* Justo EN el final del bloque sólo cabe un motor sin estáticos; uno con
     * `.data`/`.bss` sellado ahí es que el bloque ENCOGIÓ desde que se grabó, y
     * eso es «regrábalo», no «no cabe» — mandan a sitios distintos. */
    if (h->linked_ram < ram_lo || h->linked_ram > ram_hi ||
        (h->linked_ram == ram_hi && (h->data_bytes | h->bss_bytes) != 0))
                                                  return BPVM_NPACK_E_SELLO;

    /* 6 — tamaños. Antes de copiar nada, que quepa; y que la entrada apunte
     *     DENTRO del código, no más allá. Las sumas se comprueban restando: una
     *     cabecera corrupta con tamaños enormes desbordaría un uint32_t y
     *     pasaría por pequeña. */
    if (h->flash_bytes > sitio_flash)             return BPVM_NPACK_E_TAMANO;
    if (h->data_bytes > sitio_flash - h->flash_bytes)
                                                  return BPVM_NPACK_E_TAMANO;
    if (h->data_bytes > 0xFFFFFFFFu - h->bss_bytes) return BPVM_NPACK_E_TAMANO;
    if (h->data_bytes + h->bss_bytes > ram_hi - h->linked_ram)
                                                  return BPVM_NPACK_E_TAMANO;
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

uint32_t bpvm_npack_ram_bytes(const bpvm_npack_hdr_t* h)
{
    if (h == 0) return 0;
    /* Con el mismo cuidado que la escalera: una cabecera rota no puede
     * desbordar la suma y parecer un motor de 8 bytes. */
    if (h->data_bytes > 0xFFFFFFF0u - h->bss_bytes) return 0xFFFFFFF0u;
    return (h->data_bytes + h->bss_bytes + 15u) & ~15u;
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

int bpvm_npack_cargable(const bpvm_npack_carga_t* c)
{
    return c != 0 && c->motivo == BPVM_NPACK_OK
        && c->eclipsado_por < 0 && c->pisa_a < 0;
}

/* ¿Se solapan [a, a+na) y [b, b+nb)? Un trozo vacío no pisa a nadie. */
static int solapan(uint32_t a, uint32_t na, uint32_t b, uint32_t nb)
{
    if (na == 0 || nb == 0) return 0;
    return a < b + nb && b < a + na;
}

/*
 * El plan. Ver bpvm_npack.h. Dos pasadas, y la razón de que sean dos: el
 * «gana el de más adelante» necesita ver la cadena ENTERA antes de decidir
 * sobre el primero, y el solape sólo tiene sentido entre los que de verdad se
 * van a cargar — que no se sabe hasta resolver lo anterior.
 */
int bpvm_npack_planear(const uint8_t* zona, uint32_t zbytes,
                       uint32_t ram_base, uint32_t ram_bytes,
                       const char* bios_falta,
                       bpvm_npack_carga_t* out, int max,
                       bpvm_npack_plan_t* plan)
{
    int      vistos = 0, n = 0, cargables = 0;
    uint32_t tope   = ram_base;
    /* El fin del bloque. Un bloque que llegara al final del espacio de
     * direcciones desbordaría la suma: se recorta en vez de dar la vuelta. */
    uint32_t ram_fin = (ram_bytes > 0xFFFFFFFFu - ram_base) ? 0xFFFFFFFFu
                                                            : ram_base + ram_bytes;
    if (out == 0) max = 0;

    /* ── 1ª pasada: la cadena, en orden, y el tope con TODOS los vivos ── */
    if (zona != 0 && zbytes != 0) {
        bpvm_pack_src_t src;
        bpvm_pack_src_mem(&src, zona, zbytes);
        for (int32_t pc = bpvm_pack_iter(&src, 0); pc > 0; pc = bpvm_pack_iter(&src, pc)) {
            bpvm_pack_info_t info;
            if (bpvm_pack_iter_info(&src, pc, &info) != 0 || !info.alive) continue;

            for (int32_t ec = bpvm_pack_iter_entry(&src, pc, 0); ec > 0;
                 ec = bpvm_pack_iter_entry(&src, pc, ec)) {
                bpvm_pack_entry_t e;
                if (bpvm_pack_iter_entry_info(&src, pc, ec, &e) != 0) continue;
                if (!igual(e.tipo, "npk")) continue;
                vistos++;

                /* La cabecera, EN SITIO: la zona es memoria (XIP o mapeada), y
                 * las entradas van alineadas a 4 — se lee como struct. */
                const bpvm_npack_hdr_t* h = 0;
                if (e.len >= BPVM_NPACK_HDR_BYTES)
                    h = (const bpvm_npack_hdr_t*) (const void*) (zona + e.data_off);

                /* El tope: todo sello vivo que cae entero dentro del bloque. Se
                 * mira aunque el motor no se vaya a cargar (otra arquitectura,
                 * eclipsado, pasado de `max`): mientras el pack viva, ese trozo
                 * es suyo y el siguiente que se grabe tiene que ir por encima. */
                if (h != 0 && h->magic == BPVM_NPACK_MAGIC && ram_base != 0) {
                    uint32_t rb = bpvm_npack_ram_bytes(h);
                    uint32_t exacto = rb;   /* lo que tiene que caber: sin redondear */
                    if (h->data_bytes <= 0xFFFFFFFFu - h->bss_bytes)
                        exacto = h->data_bytes + h->bss_bytes;
                    if (h->linked_ram >= ram_base && h->linked_ram <= ram_fin &&
                        exacto <= ram_fin - h->linked_ram) {
                        uint32_t fin = (rb > ram_fin - h->linked_ram) ? ram_fin
                                                                      : h->linked_ram + rb;
                        if (fin > tope) tope = fin;
                    }
                }

                if (n >= max) continue;              /* se cuenta, no se describe */
                bpvm_npack_carga_t* c = &out[n++];
                c->hdr           = h;
                c->codigo        = (uint32_t) (uintptr_t) (zona + e.data_off) + BPVM_NPACK_HDR_BYTES;
                c->pack_off      = info.off;
                copia(c->pack, info.nombre, (int) sizeof c->pack);
                copia(c->npk,  e.nombre,    (int) sizeof c->npk);
                c->ram           = h ? h->linked_ram : 0;
                c->ram_bytes     = h ? bpvm_npack_ram_bytes(h) : 0;
                c->eclipsado_por = -1;
                c->pisa_a        = -1;
                /* Una entrada `npk` más corta que su cabecera no es un motor:
                 * mismo peldaño que un magic malo. */
                c->motivo = h ? bpvm_npack_escalera(h, c->codigo,
                                                    e.len - BPVM_NPACK_HDR_BYTES,
                                                    ram_base, ram_fin, bios_falta)
                              : BPVM_NPACK_E_MAGIC;
            }
        }
    }

    /* ── 2ª pasada: gana el de más adelante, y nadie pisa a nadie ── */
    for (int i = 0; i < n; i++) {
        for (int j = n - 1; j > i; j--)
            if (igual(out[i].npk, out[j].npk)) { out[i].eclipsado_por = (int8_t) j; break; }
    }
    for (int i = 0; i < n; i++) {
        if (out[i].motivo != BPVM_NPACK_OK || out[i].eclipsado_por >= 0) continue;
        /* Contra los cargables ANTERIORES: en orden de cadena, el primero que
         * se grabó se queda su trozo y el que llega después es el que sobra. */
        for (int j = 0; j < i; j++) {
            if (!bpvm_npack_cargable(&out[j])) continue;
            if (solapan(out[i].ram, out[i].ram_bytes, out[j].ram, out[j].ram_bytes)) {
                out[i].pisa_a = (int8_t) j;
                break;
            }
        }
        if (out[i].pisa_a < 0) cargables++;
    }

    /* El tope, alineado — sin salirse del bloque. */
    tope = (tope > 0xFFFFFFF0u) ? tope : ((tope + 15u) & ~15u);
    if (tope > ram_fin) tope = ram_fin;

    if (plan) {
        plan->vistos    = vistos;
        plan->cargables = cargables;
        plan->ram_tope  = tope;
    }
    return vistos;
}

uint32_t bpvm_npack_ram_tope(const uint8_t* zona, uint32_t zbytes,
                             uint32_t ram_base, uint32_t ram_bytes)
{
    if (ram_base == 0) return 0;
    bpvm_npack_plan_t p;
    bpvm_npack_planear(zona, zbytes, ram_base, ram_bytes, 0, 0, 0, &p);
    return p.ram_tope;
}
