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

/* V5/H7 (16-ago) - ¿HAY ALGO QUE BUSCAR? Sin ningún pack grabado no hay cadena
 * que recorrer, y eso es barato de saber — `bpvm_pack_scan` lee la primera
 * cabecera y para. (Nació para ahorrarse el BARRIDO de toda la zona de 4 en 4
 * bytes, 338 ms medidos en el P4 en cada intento; desde G1 ya no se barre, pero
 * la pregunta sigue siendo la primera que hay que hacer.) */
static int hay_algun_pack(const uint8_t* base, uint32_t bytes) {
    if (base == 0 || bytes == 0) return 0;
    uint32_t fin = 0;
    return bpvm_pack_scan(base, bytes, 0, 0, 0, &fin) > 0;
}

void pack_pico_montar(void)
{
    const bpvm_part_layout_t* lay = board_partitions();
    const bpvm_part_t* pp = lay ? bpvm_part_get(lay, BPVM_PART_PACKS) : 0;
    if (!pp || pp->size == 0) return;
    const void* base = (const void*) (uintptr_t) (XIP_BASE + pp->offset);
    bpvm_pack_mount((const uint8_t*) base, (const uint8_t*) base, (uint32_t) pp->size);
    log_printf("pack: zona montada en 0x%08lX (%u KB) — modulos y .mdn visibles",
               (unsigned long) (uintptr_t) base, (unsigned) (pp->size / 1024));
}

/* Por qué un motor no se carga — con los números cuando los hay. */
static void explicar(const bpvm_npack_carga_t* c, int i, const bpvm_npack_carga_t* todos)
{
    if (c->eclipsado_por >= 0) {
        log_printf("pack: [%d] '%s' (motor '%s') NO se carga: hay otro motor '%s' mas adelante "
                   "(el pack '%s', grabado despues) y gana el ultimo, como los .mod — borra el viejo",
                   i, c->pack, c->npk, c->npk, todos[c->eclipsado_por].pack);
        return;
    }
    if (c->pisa_a >= 0) {
        const bpvm_npack_carga_t* o = &todos[c->pisa_a];
        log_printf("pack: [%d] '%s' NO se carga: su RAM [0x%08lX, +%u) pisa la de '%s' [0x%08lX, +%u) "
                   "— se grabo con una imagen de antes de G1 (daba la base a todos): regrabalo",
                   i, c->pack, (unsigned long) c->ram, (unsigned) c->ram_bytes,
                   o->pack, (unsigned long) o->ram, (unsigned) o->ram_bytes);
        return;
    }
    log_printf("pack: [%d] '%s' (motor '%s') NO se carga — %s",
               i, c->pack, c->npk, bpvm_npack_res_str(c->motivo));
    if (c->hdr == 0) return;
    if (c->motivo == BPVM_NPACK_E_SELLO)
        log_printf("pack: [%d] su sello dice flash 0x%08lX ram 0x%08lX | aqui esta en flash 0x%08lX "
                   "y el bloque de RAM es [0x%08lX, +%u)",
                   i, (unsigned long) c->hdr->linked_flash, (unsigned long) c->hdr->linked_ram,
                   (unsigned long) c->codigo, (unsigned long) (uintptr_t) s_pack_ram_base,
                   (unsigned) s_pack_ram_size);
    else if (c->motivo == BPVM_NPACK_E_TAMANO)
        log_printf("pack: [%d] pide .data %u + .bss %u B desde 0x%08lX; el bloque acaba en 0x%08lX",
                   i, (unsigned) c->hdr->data_bytes, (unsigned) c->hdr->bss_bytes,
                   (unsigned long) c->hdr->linked_ram,
                   (unsigned long) ((uintptr_t) s_pack_ram_base + s_pack_ram_size));
}

/* Saltar a UN motor: su RAM (la `.data` copiada, la `.bss` a cero, EN SU
 * TROZO), la miga de pan, y la arena con dueño. Si esto no se hiciera, sus
 * variables arrancarían con basura. */
static int32_t saltar(const bpvm_npack_carga_t* c, int i, const bpvm_bios_t* bios)
{
    const bpvm_npack_hdr_t* hdr = c->hdr;
    uint8_t* ram = (uint8_t*) (uintptr_t) c->ram;
    const uint8_t* data_img = (const uint8_t*) (uintptr_t) (c->codigo + hdr->flash_bytes);
    if (hdr->data_bytes) memcpy(ram, data_img, hdr->data_bytes);
    if (hdr->bss_bytes)  memset(ram + hdr->data_bytes, 0, hdr->bss_bytes);

    uint32_t entrada = bpvm_npack_entry_addr(hdr, c->codigo);

    /* LA MIGA DE PAN. Antes de saltar. Si lo siguiente que ves en el log de un
     * arranque posterior NO es "vuelto", se colgó aquí. */
    log_printf("pack: [%d] '%s' (motor '%s') en 0x%08lX, .data %u + .bss %u a 0x%08lX",
               i, c->pack, c->npk, (unsigned long) (c->codigo - BPVM_NPACK_HDR_BYTES),
               (unsigned) hdr->data_bytes, (unsigned) hdr->bss_bytes,
               (unsigned long) c->ram);
    log_printf("pack: [%d] SALTANDO a 0x%08lX ...", i, (unsigned long) entrada);

    bpvm_bios_arena_entra(c->pack);
    typedef int (*entrada_fn)(const bpvm_bios_t*);
    int r = ((entrada_fn) (uintptr_t) entrada)(bios);
    bpvm_bios_arena_sale();

    const char* no = bpvm_bios_arena_negada();
    log_printf("pack: [%d] '%s' vuelto, devolvio %d%s%s", i, c->pack, r,
               no ? " | pidio la arena y se le nego: " : "", no ? no : "");
    return (int32_t) r;
}

/*
 * Carga y ejecuta los motores de la zona. Devuelve:
 *
 *    >= 1  a cuántos se SALTÓ (lo que devolvió cada uno está en el log)
 *    <  0  no se saltó a ninguno; `-valor` es el peldaño del último rechazado
 *          (bpvm_npack_res_t), o MAGIC si no había ninguno
 *
 * Los dos rangos no se solapan porque los peldaños son 1..8. El detalle —qué
 * motor, por qué, con qué números— va al LOG, que es donde se diagnostica.
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
    const uint8_t* base  = (const uint8_t*) (uintptr_t) (XIP_BASE + pp->offset);
    uint32_t       bytes = (uint32_t) pp->size;

    /* 1b — MONTAR la zona, que es cosa aparte de buscar código nativo en ella.
     *
     * Un pack puede llevar tres cosas: código nativo (`npk`), módulos BP (`mod`)
     * y puentes AOT (`mdn`). Los motores los recorre el plan —justo abajo—; los
     * otros dos los busca la VM por `bpvm_pack_mounted()`, y si nadie ha
     * montado, ahí no hay nada que encontrar.
     *
     * ⚠️ Esto FALTABA, y no era una regresión: el camino «cargar un módulo desde
     * la zona XIP» no se había ejercitado nunca en esta familia. #310 corre packs
     * del FS por streaming (`run_pack_src`, otro camino) y #327 verificó grabar y
     * persistir, no ejecutar desde la zona. Se vio en placa: `falta el modulo
     * 'SQLite'` teniendo el .mod dentro del pack.
     *
     * Va ANTES del plan y pase lo que pase con él: un pack de sólo módulos, sin
     * código nativo, tiene que valer igual. */
    bpvm_pack_mount(base, base, bytes);
    log_printf("pack: zona montada en 0x%08lX (%u KB) — modulos y .mdn visibles",
               (unsigned long) (uintptr_t) base, (unsigned) (bytes / 1024));

    /* 2 — la BIOS que le vamos a prestar, ANTES de nada. Si tiene huecos, el
     *     pack se colgaría DENTRO, donde no hay depurador. */
    const bpvm_bios_t* bios = bios_pico_get();   /* él ya loguea qué falta */
    const char* falta = bios ? 0 : "tabla";

    /* 3 — ¿hay sitio donde poner los estáticos? Lo decide el arranque: el
     *     bloque de la BD, o la cola de la SRAM si no hay BD. */
    if (s_pack_ram_base == 0 || s_pack_ram_size == 0) {
        log_printf("pack: no hay RAM reservada para los motores (ni arena ni SRAM)");
        return -(int32_t) BPVM_NPACK_E_TAMANO;
    }

    /* 3.5 — ¿hay algo grabado? Si no, no hay cadena que recorrer. */
    if (!hay_algun_pack(base, bytes)) {
        log_printf("pack: la zona no tiene ningun pack grabado - no se recorre");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }

    /* 4 — EL PLAN: la cadena, en orden, con el veredicto de cada motor. */
    uint32_t ram_base = (uint32_t) (uintptr_t) s_pack_ram_base;
    bpvm_npack_carga_t c[BPVM_NPACK_MAX];
    bpvm_npack_plan_t  p;
    int vistos = bpvm_npack_planear(base, bytes, ram_base, s_pack_ram_size, falta,
                                    c, BPVM_NPACK_MAX, &p);
    int n = (vistos < BPVM_NPACK_MAX) ? vistos : BPVM_NPACK_MAX;
    if (vistos == 0) {
        log_printf("pack: hay packs grabados pero ninguno trae motor nativo (npk)");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }
    uint32_t usada = p.ram_tope - ram_base;
    log_printf("pack: %d motor(es), %d cargable(s) | bloque %u KB @0x%08lX: estaticos %u B, arena %u KB",
               vistos, p.cargables, (unsigned) (s_pack_ram_size / 1024u),
               (unsigned long) ram_base, (unsigned) usada,
               (unsigned) ((s_pack_ram_size - usada) / 1024u));
    if (vistos > n)
        log_printf("pack: hay %d motores y solo se miran los %d primeros (BPVM_NPACK_MAX)", vistos, n);

    /* 5 — la arena: lo que queda por encima del último motor. */
    bpvm_bios_arena_fija(s_pack_ram_base + usada, s_pack_ram_size - usada);

    /* 6 — saltar a los cargables, en el orden en que se grabaron. */
    int saltados = 0;
    bpvm_npack_res_t ultimo = BPVM_NPACK_E_MAGIC;
    for (int i = 0; i < n; i++) {
        if (!bpvm_npack_cargable(&c[i])) {
            explicar(&c[i], i, c);
            if (c[i].eclipsado_por < 0)
                ultimo = (c[i].pisa_a >= 0) ? BPVM_NPACK_E_SELLO : c[i].motivo;
            continue;
        }
        (void) saltar(&c[i], i, bios);
        saltados++;
    }
    return saltados > 0 ? saltados : -(int32_t) ultimo;
}
