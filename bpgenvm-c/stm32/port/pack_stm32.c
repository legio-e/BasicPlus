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

/* V7/H7 (Boot 2, fase 2) — EL CARGADOR, común (src/bpvm_npack_carga.c): el plan, la arena,
 * la `.data` y la `.bss`, la miga de pan y la guarda «una vez». La zona la montó el arranque
 * (fase 1: con el FS). Lo de esta placa son tres datos; el porqué del arch/ABI que aquí se
 * añadía al explicar un rechazo es portable y ya lo dice el común. */
static void log_linea(const char* l) { log_printf("%s", l); }

int32_t pack_stm32_cargar(void)
{
    bpvm_npack_familia_t f;
    memset(&f, 0, sizeof f);
    f.bios      = bios_stm32_get();     /* él ya loguea qué falta */
    f.ram       = s_pack_ram_base;      /* delante de la VM, en 0x20000000 (pack_stm32.h) */
    f.ram_bytes = s_pack_ram_size;
    f.log       = log_linea;
    f.flush     = log_flush;
    return bpvm_npack_cargar_zona(&f);
}
