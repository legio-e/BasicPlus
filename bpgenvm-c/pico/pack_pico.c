/*
 * pack_pico.c — V5/I: el CARGADOR de packs nativos de esta placa.
 *
 * Recorre la cadena de packs de la zona XIP, sube la escalera de cada motor, y
 * —sólo a los que la superan entera— salta a su entrada.
 *
 * ─── POR QUÉ ESTO VIVE EN EL FIRMWARE Y NO EN EL `.mdn` ───
 *
 * La idea original era que lo hiciera el propio código nativo del `.mdn`, que ya
 * sabe barrer memoria (así encuentra el ancla). Pero un `.mdn` NO PUEDE llamar a
 * nada del firmware por nombre —es zero-copy y tiene que ser 100 % relocatable—,
 * así que tendría que llevar su propia copia de `bpvm_npack_escalera`.
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
 *
 * ─── V7/H6.c G1: TODOS LOS MOTORES, NO EL PRIMERO ───
 *
 * Hasta G1 se cargaba UN motor —el primero que aparecía barriendo la zona— y los
 * 8 KB del principio del bloque eran suyos. Ahora se recorre la cadena y se
 * salta a todos los que el plan (`bpvm_npack_planear`, común) da por buenos,
 * cada uno a su trozo de RAM; la arena es lo que queda por encima del último.
 * ⚠️ La Metro no estaba conectada el día de G1: este fichero se compiló, pero la
 * prueba en placa de los varios motores se hizo en la familia ESP32.
 *
 * ─── V7/H7 (Boot 2, fase 2): EL CARGADOR ES COMÚN ───
 *
 * Lo de arriba sigue valiendo, pero ya no vive aquí: la misma copia estaba en las tres
 * familias y ahora es src/bpvm_npack_carga.c. Aquí quedan el montaje de la zona (el gancho
 * de la fase 1) y los tres datos de esta placa.
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
uint32_t s_pack_ram_size = 0;   /* V7/H6.c G1: el bloque que empieza ahí */

int pack_pico_montar(void)
{
    const bpvm_part_layout_t* lay = board_partitions();
    const bpvm_part_t* pp = lay ? bpvm_part_get(lay, BPVM_PART_PACKS) : 0;
    if (!pp || pp->size == 0) {
        log_printf("packs: sin zona de packs (no hay particion de packs en el reparto)");
        return 0;
    }
    const void* base = (const void*) (uintptr_t) (XIP_BASE + pp->offset);
    bpvm_pack_mount((const uint8_t*) base, (const uint8_t*) base, (uint32_t) pp->size);
    log_printf("pack: zona montada en 0x%08lX (%u KB) — modulos y .mdn visibles",
               (unsigned long) (uintptr_t) base, (unsigned) (pp->size / 1024));
    return 1;
}

/* V7/H7 (Boot 2, fase 2) — EL CARGADOR, común (src/bpvm_npack_carga.c): el plan, la arena,
 * la `.data` y la `.bss`, la miga de pan —ahora también volcada a la flash antes de saltar,
 * como en la ESP32 y la STM32— y la guarda «una vez». Lo de esta placa son tres datos. */
static void log_linea(const char* l) { log_printf("%s", l); }

int32_t pack_pico_cargar(void)
{
    bpvm_npack_familia_t f;
    memset(&f, 0, sizeof f);
    f.bios      = bios_pico_get();      /* él ya loguea qué falta */
    f.ram       = s_pack_ram_base;      /* el bloque de la BD, o la cola de la SRAM */
    f.ram_bytes = s_pack_ram_size;
    f.log       = log_linea;
    f.flush     = log_flush;
    return bpvm_npack_cargar_zona(&f);
}
