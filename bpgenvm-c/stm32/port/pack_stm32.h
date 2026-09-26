/*
 * pack_stm32.h — V7/H6 G5.b0: los packs NATIVOS en el STM32 (el cargador y su
 * bloque de RAM). Hermano de pico/pack_pico.h y esp32/common/pack_esp32.h.
 *
 * ─── DÓNDE VIVE LA RAM DE LOS MOTORES, Y POR QUÉ AHÍ ───
 *
 * El SELLO de un motor son dos direcciones —flash y RAM— y las dos se fijan en el
 * PC al grabarlo. La de flash no se mueve (la zona de packs está en BP_DATA, fuera
 * del firmware). La de RAM tiene que estar igual de quieta, y en el STM32 la VM es
 * un array estático que, en la `.bss`, se movía con cada enlace: cada imagen nueva
 * habría obligado a regrabar los packs.
 *
 * Así que el `.ld` de cada placa pone ese array (`s_vm_mem`, sección `.bss.bp_vm`)
 * LO PRIMERO de la RAM —`0x20000000` siempre—, y el bloque de los motores es la
 * RESERVA CON NOMBRE que el planificador común (`bpvm_mem`) aparta DELANTE de la
 * VM: la misma con la que la Metro aparta la BD. Su tamaño lo dice el ENV:
 *
 *   packRam=<KB>   la clave genérica (la de la ESP32): lo que se pide, en páginas
 *                  de 4 KB, si deja a la VM su suelo;
 *   SQLite=<MB>    si no hay packRam, la regla de la BD (bpvm_sqlmem): 2 MB de
 *                  mínimo, que en la Nucleo no caben y el arranque lo dice.
 *
 * Cambiar `packRam` sólo mueve el FINAL del bloque: los motores ya grabados siguen
 * valiendo mientras quepan (y si no, la escalera lo dice: «regrábalo»).
 */
#ifndef PACK_STM32_H
#define PACK_STM32_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* El bloque de RAM del código nativo: `[estáticos de cada motor | arena]`. Lo
 * fija el arranque (stm32_repl.c, al planificar la memoria). 0 = sin bloque, y
 * entonces ningún motor puede cargar (y `PACK_BURN_BEGIN` dice ramBase 0). */
extern uint8_t* s_pack_ram_base;
extern uint32_t s_pack_ram_size;

/* Carga y ejecuta los motores de la zona (todos los cargables, en el orden de la
 * cadena). Lo dispara el PRIMER Run, no el arranque (la razón, en pack_pico.c).
 *   >= 1  a cuántos motores se saltó (lo que devolvió cada uno, en el log)
 *   <  0  no se saltó a ninguno; -valor es el peldaño (bpvm_npack_res_t) */
int32_t pack_stm32_cargar(void);

/* La BIOS de esta familia (bios_stm32.c): la tabla lista para prestar, o NULL si
 * tiene huecos (y el log dice cuál); y su ancla, para que el arranque compare lo
 * que ENCUENTRA barriendo con lo que SABE. */
struct bpvm_bios;
struct bpvm_ancla;
const struct bpvm_bios*  bios_stm32_get(void);
const struct bpvm_ancla* bios_stm32_ancla(void);

#ifdef __cplusplus
}
#endif

#endif /* PACK_STM32_H */
