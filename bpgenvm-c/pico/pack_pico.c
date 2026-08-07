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
#include "bpvm_bios.h"
#include "bpvm_part.h"
#include "log.h"

#include <string.h>
#include "hardware/flash.h"     /* XIP_BASE */

const bpvm_bios_t*  bios_pico_get(void);
const bpvm_part_layout_t* board_partitions(void);

/*
 * RAM del pack: donde van su `.data` y su `.bss`.
 *
 * ⚠️ PROVISIONAL, y a propósito pequeño. El pack mínimo necesita 8 bytes; SQLite
 * necesitará bastante más y saldrá de la arena del ENV (`SQLite=<MB>`), no de
 * aquí. Este bloque existe para que el PRIMER pack tenga dónde vivir sin abrir
 * todavía la discusión de la arena — y si algún día no cabe, la escalera lo dice
 * con su peldaño TAMAÑO en vez de pisar memoria de otro.
 */
#define PACK_RAM_BYTES 1024
static uint8_t s_pack_ram[PACK_RAM_BYTES] __attribute__((aligned(8)));

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

    /* 2 — la BIOS que le vamos a prestar, ANTES de nada. Si tiene huecos, el
     *     pack se colgaría DENTRO, donde no hay depurador. */
    const bpvm_bios_t* bios = bios_pico_get();   /* él ya loguea qué falta */
    const char* falta = bios ? 0 : "tabla";

    /* 3 — BUSCARLO, no acertar dónde está (idea de Eduardo). */
    bpvm_npack_hallazgo_t h = bpvm_npack_buscar(
            base, bytes,
            (uint32_t) (uintptr_t) s_pack_ram, PACK_RAM_BYTES, falta);

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

    /* 4 — la RAM del pack: copiar su `.data` y poner su `.bss` a cero. Si esto
     *     no se hiciera, sus variables arrancarían con basura. */
    const uint8_t* data_img = (const uint8_t*) (uintptr_t) (code + hdr->flash_bytes);
    if (hdr->data_bytes) memcpy(s_pack_ram, data_img, hdr->data_bytes);
    if (hdr->bss_bytes)  memset(s_pack_ram + hdr->data_bytes, 0, hdr->bss_bytes);

    uint32_t entrada = bpvm_npack_entry_addr(hdr, code);

    /* 5 — LA MIGA DE PAN. Antes de saltar. Si lo siguiente que ves en el log de
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
