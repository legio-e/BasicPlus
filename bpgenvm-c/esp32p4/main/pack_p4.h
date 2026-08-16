/* pack_p4.h — el reparto de RAM del pack nativo en el ESP32-P4 (V5/H7).
 *
 * Hermano de `pico/pack_pico.h`, y las diferencias son del silicio, no de gusto.
 */
#ifndef PACK_P4_H
#define PACK_P4_H

#include <stdint.h>

struct bpvm_bios;

/* La tabla BIOS de esta placa, YA VERIFICADA (NULL si tiene huecos, y en ese
 * caso `bios_p4.c` ya dejó dicho en el log cuál falta). Se declara aquí porque
 * la usan los dos que van juntos: el arranque, para comprobarla, y el cargador,
 * para prestársela al pack. */
const struct bpvm_bios* bios_p4_get(void);

/*
 * Lo que el firmware OFRECE al pack para sus estáticos, NO lo que el pack pide:
 * eso lo declara él en su cabecera (`data_bytes` + `bss_bytes`) y la escalera de
 * `bpvm_npack_check` los compara. Si un pack pidiera más, sale con su peldaño
 * TAMAÑO en vez de escribir fuera — o sea que quedarse corto NO corrompe nada,
 * sólo hace que el pack no cargue.
 *
 * ─── POR QUÉ 16 KB Y NO LOS 8192 DE LA PICO ───
 *
 * Porque el pack de RISC-V MIDE MÁS, y está medido (12-ago, `sqlite_rv.npack`):
 *
 *     .data 6.352 + .sdata   564 = 6.916 B de imagen inicial
 *     .sbss    68 + .bss   1.300 = 1.368 B a cero
 *                               ── 8.284 B
 *
 * Con los 8192 de la Pico el pack RISC-V sería RECHAZADO — por 92 bytes. Y el
 * mismo número ya se quedó corto antes: 7168 se pasó por OCHO al añadirle el
 * VFS. Dos veces justo es una advertencia, no una casualidad.
 *
 * ⚠️ `.sdata`/`.sbss` son de RISC-V y NO EXISTEN en ARM (datos pequeños). Es
 * justo el tipo de sección que se olvida al portar y que aquí ya está contada.
 *
 * 16 KB da ~2x sobre lo medido. El coste es 16 KB de un bloque de 4 MB en PSRAM
 * — el 0,4 % — así que aquí el margen es gratis, al revés que en la Pico, donde
 * podía salir de la SRAM. Criterio de Eduardo (12-ago): *"16K está bien, con
 * margen para crecer si es necesario"*.
 */
#define PACK_RAM_BYTES   16384u

/*
 * La base REAL de esa zona, decidida en el ARRANQUE: el principio del bloque de
 * la BD (`[estáticos | arena]`). 0 = no hay (no se pidió BD o no cupo).
 *
 * ⚠️ Es la dirección contra la que el pack tiene que estar SELLADO, así que
 * tiene que ser la MISMA en cada arranque. Por eso el bloque de la BD se reserva
 * ANTES que el heap de la VM en `app_main`: si saliera de lo que quede libre, la
 * daría el alocador y podría cambiar — y entonces un pack grabado funcionaría
 * una vez y luego no, que es el peor síntoma posible.
 */
extern uint8_t* s_pack_ram_base;
/* V5/H8 — tamaño del bloque de la BD, del que `s_pack_ram_base` es el
 * principio. Sale al wire (`PACK_BURN_BEGIN`) como el sitio de RAM que
 * tiene el motor: el sello del `.npk` se comprueba contra él. */
extern uint32_t s_sqlite_size;

/*
 * Carga y ejecuta el pack nativo de la zona de packs.
 *
 *   >= 0  se saltó, y es lo que devolvió el pack
 *   <  0  no se saltó; -valor es el peldaño (bpvm_npack_res_t)
 */
int32_t pack_p4_cargar(void);

/*
 * Mapea la zona de packs y la PUBLICA (para el IDE y para que se vean los .mod
 * y .mdn que lleve dentro). Lo llama el arranque; es barato y no salta a nada.
 * 0 si la zona quedó mapeada, -1 si no hay zona o falló el mapeo.
 *
 * Lo otro -buscar el ancla y saltar- es `pack_p4_cargar`, y se dispara en el
 * primer `Run` (ver repl_esp32.c): es el único paso que puede colgar, y un
 * cuelgue en el arranque se repite en cada arranque.
 */
int32_t pack_p4_mapear(void);

#endif /* PACK_P4_H */
