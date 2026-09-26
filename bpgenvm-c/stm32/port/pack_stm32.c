/*
 * pack_stm32.c — V7/H6 G5.b0: el CARGADOR de packs nativos de la familia STM32.
 *
 * Hermano de `pico/pack_pico.c`, y casi calcado: en los dos la flash está XIP en
 * una dirección FIJA (aquí `FLASH_BASE` + el offset de la partición de packs) y la
 * misma vista sirve para leer y para ejecutar, así que no hay mapeo que hacer como
 * en la ESP32. Lo demás —la escalera, el sello, el plan de varios motores, la
 * arena con dueño— es el MISMO código común (`bpvm_npack.c`, `bpvm_bios.c`): esas
 * reglas no pueden tener una copia por familia.
 *
 * Lo que sí es de aquí, y conviene no olvidar:
 *
 *   · la RAM de los estáticos es la reserva que el planificador aparta DELANTE de
 *     la VM, en `0x20000000` (ver pack_stm32.h: por qué ahí y por qué fija);
 *   · la flash la escribe `stm32_flash.c`, que INVALIDA el ICACHE tras cada
 *     borrado y cada escritura: un motor recién grabado se lee y se ejecuta sin
 *     restos de lo que había antes en la caché;
 *   · se dispara en el PRIMER Run y no en el arranque (Eduardo, 7-ago: un
 *     cuelgue en un Run se arregla desenchufando una vez; en el arranque, se
 *     repite en cada arranque), y deja MIGA DE PAN en el log antes y después
 *     de saltar, volcada a la flash: si se cuelga dentro, el log del siguiente
 *     arranque enseña el «saltando» sin su «volvió».
 */
#include "pack_stm32.h"
#include "board_mgr_stm32.h"   /* board_mgr_stm32_packs: la partición del layout del boot */
#include "bpvm_npack.h"
#include "bpvm_pack.h"
#include "bpvm_bios.h"
#include "bpvm_part.h"
#include "mdn_loader.h"         /* bpvm_mdn_host_arch / _float_abi: lo que ESTA placa es */
#include "log.h"

#include "main.h"               /* FLASH_BASE */
#include <string.h>             /* memcpy/memset: la .data y la .bss de cada motor */

uint8_t* s_pack_ram_base = 0;
uint32_t s_pack_ram_size = 0;

/* ¿Hay algo grabado? `bpvm_pack_scan` lee la primera cabecera y para: sin ningún
 * pack no hay cadena que recorrer. */
static int hay_algun_pack(const uint8_t* base, uint32_t bytes) {
    if (base == 0 || bytes == 0) return 0;
    uint32_t fin = 0;
    return bpvm_pack_scan(base, bytes, 0, 0, 0, &fin) > 0;
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
                   "— dos sellos solapados: regrabalo",
                   i, c->pack, (unsigned long) c->ram, (unsigned) c->ram_bytes,
                   o->pack, (unsigned long) o->ram, (unsigned) o->ram_bytes);
        return;
    }
    log_printf("pack: [%d] '%s' (motor '%s') NO se carga — %s",
               i, c->pack, c->npk, bpvm_npack_res_str(c->motivo));
    if (c->hdr == 0) return;
    const bpvm_npack_hdr_t* m = c->hdr;
    if (c->motivo == BPVM_NPACK_E_SELLO)
        log_printf("pack: [%d] su sello dice flash 0x%08lX ram 0x%08lX | aqui esta en flash 0x%08lX "
                   "y el bloque de RAM es [0x%08lX, +%u)",
                   i, (unsigned long) m->linked_flash, (unsigned long) m->linked_ram,
                   (unsigned long) c->codigo, (unsigned long) (uintptr_t) s_pack_ram_base,
                   (unsigned) s_pack_ram_size);
    else if (c->motivo == BPVM_NPACK_E_TAMANO)
        log_printf("pack: [%d] pide .data %u + .bss %u B desde 0x%08lX; el bloque acaba en 0x%08lX",
                   i, (unsigned) m->data_bytes, (unsigned) m->bss_bytes,
                   (unsigned long) m->linked_ram,
                   (unsigned long) ((uintptr_t) s_pack_ram_base + s_pack_ram_size));
    else if (c->motivo == BPVM_NPACK_E_ARCH || c->motivo == BPVM_NPACK_E_ABI)
        /* El caso de G5.b0: hasta hoy las imágenes STM32 eran `hard` y los motores
         * ARM `softfp` — que se vea con los dos valores, no sólo con el peldaño. */
        log_printf("pack: [%d] el motor es arch %u, abi '%.8s' | esta placa ES arch %u, abi '%s'",
                   i, (unsigned) m->arch, m->float_abi,
                   (unsigned) bpvm_mdn_host_arch(), bpvm_mdn_host_float_abi());
}

/* Saltar a UN motor: su RAM (la `.data` copiada, la `.bss` a cero, EN SU TROZO),
 * la miga de pan, y la arena con dueño. Sin la copia, sus variables arrancarían
 * con basura (lo cazó SQLite en el P4: `libversion` no empezaba por «3.»). */
static int32_t saltar(const bpvm_npack_carga_t* c, int i, const bpvm_bios_t* bios)
{
    const bpvm_npack_hdr_t* hdr = c->hdr;
    uint8_t* ram = (uint8_t*) (uintptr_t) c->ram;
    const uint8_t* data_img = (const uint8_t*) (uintptr_t) (c->codigo + hdr->flash_bytes);
    if (hdr->data_bytes) memcpy(ram, data_img, hdr->data_bytes);
    if (hdr->bss_bytes)  memset(ram + hdr->data_bytes, 0, hdr->bss_bytes);

    uint32_t entrada = bpvm_npack_entry_addr(hdr, c->codigo);

    /* LA MIGA DE PAN, antes de saltar y VOLCADA: si lo siguiente que enseña el log
     * de un arranque posterior NO es «volvio», se colgó aquí dentro. */
    log_printf("pack: [%d] '%s' (motor '%s') en 0x%08lX, .data %u + .bss %u a 0x%08lX",
               i, c->pack, c->npk, (unsigned long) (c->codigo - BPVM_NPACK_HDR_BYTES),
               (unsigned) hdr->data_bytes, (unsigned) hdr->bss_bytes,
               (unsigned long) c->ram);
    log_printf("pack: [%d] SALTANDO a 0x%08lX ...", i, (unsigned long) entrada);
    log_flush();

    bpvm_bios_arena_entra(c->pack);
    typedef int32_t (*entrada_fn)(const bpvm_bios_t*);
    int32_t r = ((entrada_fn) (uintptr_t) entrada)(bios);
    bpvm_bios_arena_sale();

    const char* no = bpvm_bios_arena_negada();
    log_printf("pack: [%d] '%s' volvio, rc=%ld%s%s", i, c->pack, (long) r,
               no ? " | pidio la arena y se le nego: " : "", no ? no : "");
    return r;
}

int32_t pack_stm32_cargar(void)
{
    /* 1 — la zona: la partición de packs del layout del BOOT (la dice el ENV, no
     *     una constante). XIP directo: la dirección que ve la CPU es FLASH_BASE +
     *     el offset, la misma para leer y para saltar. */
    const bpvm_part_t* pp = board_mgr_stm32_packs();
    if (!pp || pp->size == 0) {
        log_printf("pack: no hay zona de packs en la tabla de particiones");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }
    const uint8_t* base  = (const uint8_t*) (uintptr_t) (FLASH_BASE + pp->offset);
    uint32_t       bytes = (uint32_t) pp->size;

    /* 1b — montada, por si acaso: la monta el arranque (board_mgr_stm32_montar_packs)
     *      y es idempotente. Los `.mod` y `.mdn` del pack la buscan ahí. */
    bpvm_pack_mount(base, base, bytes);

    /* 2 — la BIOS que se le va a prestar, ANTES de nada: un hueco se manifestaría
     *     DENTRO del motor, donde no hay depurador. */
    const bpvm_bios_t* bios = bios_stm32_get();   /* él ya loguea qué falta */
    const char* falta = bios ? 0 : "la BIOS de esta placa tiene huecos";

    /* 3 — ¿hay sitio para los estáticos? Lo decidió el arranque con el ENV. */
    if (s_pack_ram_base == 0 || s_pack_ram_size == 0) {
        log_printf("pack: no hay bloque de RAM para los motores (ni packRam= ni SQLite= en el ENV, "
                   "o no cupo) — no se carga ninguno");
        return -(int32_t) BPVM_NPACK_E_TAMANO;
    }

    /* 3.5 — ¿hay algo grabado? */
    if (!hay_algun_pack(base, bytes)) {
        log_printf("pack: la zona no tiene ningun pack grabado - no se recorre");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }

    /* 4 — EL PLAN: la cadena, en orden, con el veredicto de cada motor. La MISMA
     *     regla que usa PACK_BURN_BEGIN para decirle al IDE dónde sellar el siguiente. */
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
