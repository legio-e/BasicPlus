/*
 * pack_pico.c — V5/I: el CARGADOR de packs nativos de esta placa.
 *
 * Busca un pack en la zona XIP, sube la escalera de validación, y —sólo si la
 * supera entera— salta a su entrada.
 *
 * ─── POR QUÉ ESTO VIVE EN EL FIRMWARE Y NO EN EL `.mdn` ───
 *
 * La idea original era que lo hiciera el propio código nativo del `.mdn`, que ya
 * sabe barrer memoria (así encuentra el ancla). Pero un `.mdn` NO PUEDE llamar a
 * nada del firmware por nombre —es zero-copy y tiene que ser 100 % relocatable—,
 * así que tendría que llevar su propia copia de `bpvm_npack_check`.
 *
 * El barrido del ancla son doce líneas y calcarlas se aceptó. La escalera son
 * siete peldaños de lógica, y ESA es justo la parte cuya corrección importa: es
 * lo que decide si se salta o no. Duplicarla sería poner en dos sitios la única
 * regla que no puede divergir.
 *
 * Así que el cargador se queda aquí y el nativo lo LLAMA por el ancla — camino
 * ya probado en placa (paso 0, 7-ago).
 *
 * ─── POR QUÉ SE DISPARA DESDE UN `Run` Y NO EN EL ARRANQUE ───
 *
 * Decisión de Eduardo, y sale de lo que costó hoy: un cuelgue durante un `Run`
 * se arregla desenchufando una vez; un cuelgue en el ARRANQUE se repite en cada
 * arranque y obliga a regrabar. El salto es el único paso que puede colgar, así
 * que se dispara cuando tú quieres, no cuando la placa enciende.
 *
 * ─── LA MIGA DE PAN (#326) ───
 *
 * Se escribe en el log ANTES de saltar y DESPUÉS. Si se cuelga, el siguiente
 * arranque muestra el "voy a saltar" sin su "he vuelto", y ahí murió. Sin eso,
 * un cuelgue no dice ni siquiera si se llegó a saltar.
 */
#include "bpvm_npack.h"
#include "bpvm_pack.h"      /* V5/H4: montar la zona para que se vean los .mod y .mdn */
#include "bpvm_bios.h"
#include "bpvm_part.h"
#include "log.h"
#include "pack_pico.h"

#include <string.h>
#include "hardware/flash.h"     /* XIP_BASE */

const bpvm_bios_t*  bios_pico_get(void);
const bpvm_part_layout_t* board_partitions(void);

/* La base real la fija el arranque: el principio del bloque de la BD si lo hay,
 * o la SRAM si no. Ver pack_pico.h, que explica por qué y qué implica. */
uint8_t* s_pack_ram_base = 0;

/*
 * Carga y ejecuta el pack. Devuelve:
 *
 *    >= 0  SE SALTÓ, y esto es lo que devolvió el pack (el mínimo devuelve 0 si
 *          todo le cuadró, y 1..6 diciendo qué comprobación suya falló)
 *    <  0  NO se saltó; `-valor` es el peldaño (bpvm_npack_res_t)
 *
 * Los dos rangos no se solapan porque los peldaños son 1..8 y el pack nunca
 * devuelve negativo. El detalle —cuántos candidatos, dónde, por qué— va al LOG,
 * que es donde se diagnostica y donde ya sabemos mirar.
 */
int32_t pack_pico_cargar(void)
{
    /* 1 — ¿dónde está la zona de packs? La dice la tabla de particiones, no una
     *     constante: el reparto lo decide el ENV y puede cambiar. */
    const bpvm_part_layout_t* lay = board_partitions();
    const bpvm_part_t* pp = lay ? bpvm_part_get(lay, BPVM_PART_PACKS) : 0;
    if (!pp || pp->size == 0) {
        log_printf("pack: no hay zona de packs en la tabla de particiones");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }
    const void* base  = (const void*) (uintptr_t) (XIP_BASE + pp->offset);
    uint32_t    bytes = (uint32_t) pp->size;

    /* 1b — MONTAR la zona, que es cosa aparte de buscar código nativo en ella.
     *
     * Un pack puede llevar tres cosas: código nativo (`npk`), módulos BP (`mod`)
     * y puentes AOT (`mdn`). El nativo se encuentra BARRIENDO —justo abajo— y no
     * necesita nada más; los otros dos los busca la VM por `bpvm_pack_mounted()`,
     * y si nadie ha montado, ahí no hay nada que encontrar.
     *
     * ⚠️ Esto FALTABA, y no era una regresión: el camino «cargar un módulo desde
     * la zona XIP» no se había ejercitado nunca en esta familia. #310 corre packs
     * del FS por streaming (`run_pack_src`, otro camino) y #327 verificó grabar y
     * persistir, no ejecutar desde la zona. Se vio en placa: `falta el modulo
     * 'SQLite'` teniendo el .mod dentro del pack.
     *
     * Va ANTES del barrido y pase lo que pase con él: un pack de sólo módulos,
     * sin código nativo, tiene que valer igual. */
    bpvm_pack_mount((const uint8_t*) base, bytes);
    log_printf("pack: zona montada en 0x%08lX (%u KB) — modulos y .mdn visibles",
               (unsigned long) (uintptr_t) base, (unsigned) (bytes / 1024));

    /* 2 — la BIOS que le vamos a prestar, ANTES de nada. Si tiene huecos, el
     *     pack se colgaría DENTRO, donde no hay depurador. */
    const bpvm_bios_t* bios = bios_pico_get();   /* él ya loguea qué falta */
    const char* falta = bios ? 0 : "tabla";

    /* 3 — ¿hay sitio donde poner su `.data`/`.bss`? Lo decide el arranque:
     *     el principio del bloque de la BD, o la SRAM si no hay arena. */
    if (s_pack_ram_base == 0) {
        log_printf("pack: no hay RAM reservada para el pack (ni arena ni SRAM)");
        return -(int32_t) BPVM_NPACK_E_TAMANO;
    }
    log_printf("pack: su RAM en 0x%08lX (%u B disponibles)",
               (unsigned long) (uintptr_t) s_pack_ram_base,
               (unsigned) PACK_RAM_BYTES);

    /* 4 — BUSCARLO, no acertar dónde está (idea de Eduardo). */
    bpvm_npack_hallazgo_t h = bpvm_npack_buscar(
            base, bytes,
            (uint32_t) (uintptr_t) s_pack_ram_base, PACK_RAM_BYTES, falta);

    if (h.addr == 0) {
        /* Y aquí el chivato gana su sueldo: "no hay pack" y "hay uno pero está
         * mal" mandan a sitios completamente distintos. */
        if (h.candidatos == 0) {
            log_printf("pack: no hay ninguno grabado (barridos %u KB desde 0x%08lX)",
                       (unsigned) (bytes / 1024), (unsigned long) (uintptr_t) base);
        } else {
            log_printf("pack: hay %u candidato(s) pero NINGUNO vale -> %s",
                       (unsigned) h.candidatos, bpvm_npack_res_str(h.motivo));
        }
        return -(int32_t) h.motivo;
    }

    const bpvm_npack_hdr_t* hdr = (const bpvm_npack_hdr_t*) (uintptr_t) h.addr;
    uint32_t code = h.addr + BPVM_NPACK_HDR_BYTES;

    /* 5 — la RAM del pack: copiar su `.data` y poner su `.bss` a cero. Si esto
     *     no se hiciera, sus variables arrancarían con basura. */
    const uint8_t* data_img = (const uint8_t*) (uintptr_t) (code + hdr->flash_bytes);
    if (hdr->data_bytes) memcpy(s_pack_ram_base, data_img, hdr->data_bytes);
    if (hdr->bss_bytes)  memset(s_pack_ram_base + hdr->data_bytes, 0, hdr->bss_bytes);

    uint32_t entrada = bpvm_npack_entry_addr(hdr, code);

    /* 6 — LA MIGA DE PAN. Antes de saltar. Si lo siguiente que ves en el log de
     *     un arranque posterior NO es "vuelto", se colgó aquí. */
    log_printf("pack: hallado en 0x%08lX, codigo 0x%08lX, .data %u + .bss %u",
               (unsigned long) h.addr, (unsigned long) code,
               (unsigned) hdr->data_bytes, (unsigned) hdr->bss_bytes);
    log_printf("pack: SALTANDO a 0x%08lX ...", (unsigned long) entrada);

    typedef int (*entrada_fn)(const bpvm_bios_t*);
    int r = ((entrada_fn) (uintptr_t) entrada)(bios);

    log_printf("pack: vuelto, devolvio %d", r);
    return (int32_t) r;
}
